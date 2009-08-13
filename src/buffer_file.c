/*
 * Copyright 2008, 2009 Alexandros Frantzis, Michael Iatrou
 *
 * This file is part of libbls.
 *
 * libbls is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * libbls is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * libbls.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file buffer_file.c
 *
 * Buffer file operations
 */

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "buffer.h"
#include "buffer_options.h"
#include "buffer_internal.h"
#include "segcol_list.h"
#include "data_object.h"
#include "data_object_file.h"
#include "overlap_graph.h"
#include "list.h"
#include "buffer_util.h"
#include "util.h"
#include "type_limits.h"


#pragma GCC visibility push(default)

/********************/
/* Helper functions */
/********************/

/**
 * Checks if a file descriptor points to a resizable file.
 *
 * @param fd the fd to check
 * @param[out] fd_resizable whether the fd is resizable
 *
 * @return the operation error code
 */
static int is_fd_resizable(int fd, int *fd_resizable)
{
	struct stat stat;
	if (fstat(fd, &stat) == -1)
		return_error(errno);

	if (S_ISREG(stat.st_mode))
		*fd_resizable = 1;
	else
		*fd_resizable = 0;

	return 0;
}

/**
 * Reserves disk space for writing a file.
 *
 * @param fd the file to extend
 * @param size the space to reserve
 *
 * @return the operation error code
 */
static int reserve_disk_space(int fd, off_t size)
{
#ifdef HAVE_POSIX_FALLOCATE
	int err = posix_fallocate(fd, 0, size);
	if (err)  
		return_error(err);
	return 0;
#else
	off_t cur_size = lseek(fd, 0, SEEK_END);

	if (cur_size == -1)
		return_error(errno);

	if (cur_size >= size)
		return 0;

	size_t block_size = 4096;

	void *zero = calloc(1, block_size);
	if (zero == NULL)
		return_error(ENOMEM);
	
	off_t bytes_left = size - cur_size;
	while (bytes_left > 0) {
		off_t nwrite = bytes_left >= block_size ? block_size : bytes_left;
		write(fd, zero, nwrite);
		bytes_left -= nwrite;
	}

	return 0;
#endif
}

/** 
 * Create an overlap graph for a segcol_t.
 *
 * See doc/devel/buffer_save.txt for more.
 * 
 * @param[out] g the created overlap graph
 * @param segcol the segcol_t to create the overlap graph for
 * @param fd_obj the file data_object_t pointing to the file to check
 *               for overlap with
 * 
 * @return the operation error code
 */
static int create_overlap_graph(overlap_graph_t **g, segcol_t *segcol,
		data_object_t *fd_obj)
{
	int err = overlap_graph_new(g, 10);
	if (err)
		return_error(err);

	/* Get an iterator for segcol */
	segcol_iter_t *iter;
	err = segcol_iter_new(segcol, &iter);
	if (err) {
		overlap_graph_free(*g);
		return_error(err);
	}

	int valid;

	/* For every segment in the segcol */
	while (!segcol_iter_is_valid(iter, &valid) && valid) {

		segment_t *seg;
		segcol_iter_get_segment(iter, &seg);

		/* 
		 * If the segment points to the file we are trying
		 * to save add it to the overlap graph.
		 */
		data_object_t *dobj;
		segment_get_data(seg, (void *)&dobj);

		int result;
		data_object_compare(&result, fd_obj, dobj);

		if (result == 0) {
			off_t mapping;
			segcol_iter_get_mapping(iter, &mapping);
			overlap_graph_add_segment(*g, seg, mapping);
		}

		segcol_iter_next(iter);
	}

	segcol_iter_free(iter);

	return 0;

}


/**
 * Break an edge of the overlap graph.
 *
 * @param segcol the segcol_t containing the segments
 * @param edge the edge to break
 * @param tmpdir the directory where to store temporary files (if needed)
 *
 * @return the operation error code
 */ 
static int break_edge(segcol_t *segcol, struct edge_entry *edge, char *tmpdir)
{
	off_t src_start;
	segment_get_start(edge->src, &src_start);

	data_object_t *dst_dobj;
	segment_get_data(edge->dst, (void *)&dst_dobj);

	/* Calculate the offset of the overlap */
	off_t overlap_offset;

	/* If dst (mapping) starts before src the overlap start at src */
	if (edge->dst_mapping < src_start)
		overlap_offset = src_start;
	else
		overlap_offset = edge->dst_mapping;

	/* Try to store the data first in memory and if that fails, in a file */
	int err = segcol_store_in_memory(segcol, overlap_offset,
			edge->weight);

	if (err == ENOMEM) {
		err = segcol_store_in_file(segcol, overlap_offset,
			edge->weight, tmpdir);
	}

	if (err)
		return_error(err);

	return 0;
}

/**
 * Writes the data of a segment to a file.
 *
 * @param fd the file desciptor of the file to write to
 * @param segment the segment to write
 * @param mapping the mapping of the segment to write
 * @param overlap the overlap of the segment with itself in bytes
 *
 * @return the operation error code
 */
static int write_segment(int fd, segment_t *segment, off_t mapping, off_t overlap)
{
	int err;

	data_object_t *dobj;
	segment_get_data(segment, (void **)&dobj);

	off_t seg_start;
	segment_get_start(segment, &seg_start);

	off_t seg_size;
	segment_get_size(segment, &seg_size);

	off_t nwrite = seg_size;

	/* 
	 * if segment overlaps with itself but has moved to a lower address there
	 * is no need to do anything special. It will be written correctly by the
	 * code outside the if (overlap > 0).
	 *
	 * However if it has not moved or has moved to a higher address special
	 * actions must be taken.
	 */
	if (overlap > 0 && mapping >= seg_start) {
		/* if the segment has not moved at all, don't write anything */
		if (mapping == seg_start)
			return 0;

		/* 
		 * If the segment has moved to a higher address we will have something
		 * like this:
		 *
		 * |----seg_size-----|
		 * [AAAAA|BBBBB|CCCCC] Original File
		 * ^     [AAAAA|BBBBB|CCCCC] Buffer
		 * |     |--overlap--|
		 * |     ^
		 * |     mapping
		 * seg_start
		 *
		 * The C's are the non-overlapping part and must be written first. C's
		 * exist if the mapping is higher than seg_start. 
		 *
		 * Then if there is a B part it must be written next because if we
		 * write the A's first, the B's in the file will be overwritten and we
		 * won't be able to write them next. The B's part exists if the length
		 * of the A's part (seg_size - overlap) is less than the overlap.
		 *
		 * The A's are written by the code outside the if.
		 */
		if (mapping > seg_start) {
			/* Write the C's part */
			err = write_data_object(dobj, seg_start + overlap, 
					seg_size - overlap, fd, mapping + overlap);
			if (err)
				return_error(err);

			/* The A's part size (assuming B's do not exist) */
			nwrite = overlap;

			/* Write the B's part if it exists */
			if (seg_size - overlap < overlap) {
				err = write_data_object(dobj, seg_start + seg_size - overlap,
						2 * overlap - seg_size, fd,
						mapping + (seg_size - overlap));
				if (err)
					return_error(err);

				/* Adjust size of A's part */
				nwrite -= 2 * overlap - seg_size;
			}

		}
	}

	/* 
	 * Write the segment (or the A's part if the segment
	 * overlaps with itself and has moved to a higher address).
	 */
	err = write_data_object(dobj, seg_start, nwrite, fd, mapping);
	if (err)
		return_error(err);

	return 0;
}

/**
 * Writes the data of a segcol except those belonging to the file we are
 * trying to write to.
 *
 * @param fd the file descriptor of the file to write to
 * @param segcol the segcol to write the data of
 * @param fd_obj a data_object_t pointing to fd
 *
 * @return the operation error code
 */
static int write_segcol_rest(int fd, segcol_t *segcol, data_object_t *fd_obj)
{
	/* Get an iterator for segcol */
	segcol_iter_t *iter;
	int err = segcol_iter_new(segcol, &iter);
	if (err)
		return_error(err);

	int valid;

	/* For every segment in the segcol */
	while (!segcol_iter_is_valid(iter, &valid) && valid) {
		segment_t *seg;
		segcol_iter_get_segment(iter, &seg);

		data_object_t *dobj;
		segment_get_data(seg, (void *)&dobj);

		int result;
		data_object_compare(&result, fd_obj, dobj);

		/* 
		 * If the segment doesn't point to the file we are trying
		 * to save, write it.
		 */
		if (result == 1) {
			off_t mapping;
			segcol_iter_get_mapping(iter, &mapping);
			err = write_segment(fd, seg, mapping, 0);
			if (err) {
				segcol_iter_free(iter);
				return_error(err);
			}
		}

		segcol_iter_next(iter);
	}

	segcol_iter_free(iter);

	return 0;
}


/** 
 * Makes private copies of buffer (undo/redo) action data that belong to
 * a specific data object.
 *
 * This is done to ensure the integrity of the data in case the specified
 * data object changes (eg a file during save).
 *
 * If 'del' is non-zero and a private copy of an action can not be made, that and
 * older actions are removed from the undo/redo list. Otherwise in case a private
 * copy fails an error is immediately returned.
 *  
 * @param buf the bless_buffer_t 
 * @param obj the data_object_t the data must belong to
 * @param del whether to delete any actions (and actions older than them) that
 *            we cannot make private copies of.
 * 
 * @return the operation error code
 */
static int actions_make_private_copy(bless_buffer_t *buf, data_object_t *obj,
		int del)
{
	int err;
	struct list_node *node;
	struct list_node *tmp;
	int undo_err = 0;
	int redo_err = 0;

	/* 
	 * Try to make private copies of undo actions starting from the newest one.
	 * If a private copy of an action fails, remove that and all older actions
	 * from the undo list.
	 */
	list_for_each_reverse_safe(action_list_tail(buf->undo_list)->prev, node, tmp) {
		struct buffer_action_entry *entry =
			list_entry(node, struct buffer_action_entry , ln);

		/* If we have previously encountered an error, remove the action */
		if (undo_err) {
			list_delete_chain(node, node);
			buffer_action_free(entry->action);
			free(entry);
		} 
		else
			err = buffer_action_private_copy(entry->action, obj);

		/* 
		 * If the private copy failed remove the action and mark the undo_err.
		 * However, if the caller doesn't want us to delete anything just return
		 * an error.
		 */
		if (!undo_err && err) {
			if (!del)
				return_error(err);
			undo_err = err;
			list_delete_chain(node, node);
			buffer_action_free(entry->action);
			free(entry);
		}
	}

	/* 
	 * Try to make private copies of redo actions starting from the newest one.
	 * If a private copy of an action fails, remove that and all older actions
	 * from the redo list.
	 */
	list_for_each_safe(action_list_head(buf->redo_list)->next, node, tmp) {
		struct buffer_action_entry *entry =
			list_entry(node, struct buffer_action_entry , ln);

		/* If we have previously encountered an error, remove the action */
		if (redo_err) {
			list_delete_chain(node, node);
			buffer_action_free(entry->action);
			free(entry);
		} 
		else
			err = buffer_action_private_copy(entry->action, obj);

		/* 
		 * If the private copy failed remove the action and mark the undo_err.
		 * However, if the caller doesn't want us to delete anything just return
		 * an error.
		 */
		if (!redo_err && err) {
			if (!del)
				return_error(err);
			redo_err = err;
			list_delete_chain(node, node);
			buffer_action_free(entry->action);
			free(entry);
		}
	}

	if (undo_err)
		return_error(undo_err);

	if (redo_err)
		return_error(redo_err);

	return 0;
}

/** 
 * Creates a new buffer_options struct.
 * 
 * @param[out] opts the created buffer_options struct.
 * 
 * @return the operation error code
 */
static int buffer_options_new(struct buffer_options **opts)
{
	*opts = malloc(sizeof(**opts));
	if (*opts == NULL)
		return_error(ENOMEM);

	/* Set default values for options */
	(*opts)->tmp_dir = strdup("/tmp");
	if ((*opts)->tmp_dir == NULL)
		return_error(ENOMEM);

	(*opts)->undo_limit = __MAX(size_t);

	(*opts)->undo_limit_str = strdup("infinite");
	if ((*opts)->undo_limit_str == NULL) {
		free((*opts)->tmp_dir);
		return_error(ENOMEM);
	}

	(*opts)->undo_after_save = strdup("best_effort");
	if ((*opts)->undo_after_save == NULL) {
		free((*opts)->undo_limit_str);
		free((*opts)->tmp_dir);
		return_error(ENOMEM);
	}

	return 0;
}

/** 
 * Frees a buffer_options struct.
 * 
 * @param opts the buffer_options struct to free
 * 
 * @return the operation error code
 */
static int buffer_options_free(struct buffer_options *opts)
{
	free(opts->tmp_dir);
	free(opts->undo_limit_str);
	free(opts->undo_after_save);
	free(opts);

	return 0;
}

/*****************/
/* API functions */
/*****************/

/**
 * Creates an empty bless_buffer_t.
 *
 * @param[out] buf an empty bless_buffer_t
 *
 * @return the operation error code
 */
int bless_buffer_new(bless_buffer_t **buf)
{
	if (buf == NULL)
		return_error(EINVAL);

	*buf = malloc(sizeof **buf);

	if (*buf == NULL)
		return_error(ENOMEM);
	
	int err = segcol_list_new(&(*buf)->segcol);
	if (err)
		goto fail_segcol;

	err = buffer_options_new(&(*buf)->options);
	if (err)
		goto fail_options;
		
	err = list_new(&(*buf)->undo_list, struct buffer_action_entry, ln);
	if (err)
		goto fail_undo;

	(*buf)->undo_list_size = 0;

	err = list_new(&(*buf)->redo_list, struct buffer_action_entry, ln);
	if (err)
		goto fail_redo;

	(*buf)->redo_list_size = 0;
	(*buf)->event_func = NULL;
	(*buf)->event_user_data = NULL;

	return 0;

	/* Handle failures */
fail_redo:
	list_free((*buf)->undo_list, struct buffer_action_entry, ln);
fail_undo:
	buffer_options_free((*buf)->options);
fail_options:
	segcol_free((*buf)->segcol);
fail_segcol:
	free(buf);

	return_error(err);
}

/**
 * Saves the contents of a bless_buffer_t to a file.
 *
 * @param buf the bless_buffer_t whose contents to save
 * @param fd the file descriptor of the file to save the contents to
 * @param progress_func the bless_progress_func to call to report the 
 *                      progress of the operation or NULL to disable reporting
 *
 * @return the operation error code
 */
int bless_buffer_save(bless_buffer_t *buf, int fd,
		bless_progress_func *progress_func)
{
	if (buf == NULL)
		return_error(EINVAL);

	off_t segcol_size;
	segcol_get_size(buf->segcol, &segcol_size);

	/* 
	 * Check if file is resizable. Initialize variable just to silence
	 * compiler warning.
	 */
	int fd_resizable = 0;

	int err = is_fd_resizable(fd, &fd_resizable);
	if (err)
		return_error(err);

	/* 
	 * If fd is a resizable (eg regular) file try to reserve enough disk space
	 * to fit the buffer.
	 *
	 * If fd is not a resizable file (eg block device) just check that the buffer
	 * fits in the file.
	 */
	if (fd_resizable == 1) {
		err = reserve_disk_space(fd, segcol_size); 
		if (err)
			return_error(err); 
	} else { 
		off_t fd_size = lseek(fd, 0, SEEK_END);
		if (fd_size < segcol_size)
			return_error(ENOSPC);
	}

	/* Create a data_object_t holding fd */
	data_object_t *fd_obj;
	err = data_object_file_new(&fd_obj, fd);
	if (err)
		return_error(err);

	/* Make private copies of data in undo/redo actions. */
	if (!strcmp(buf->options->undo_after_save, "always")) {
		/* 
		 * If the policy is "always" and we cannot safely keep the whole
		 * action history, don't carry on with the save.
		 */
		err = actions_make_private_copy(buf, fd_obj, 0);
		if (err)
			goto fail1;
	}
	else if (!strcmp(buf->options->undo_after_save, "best_effort")) {
		/* 
		 * If the policy is "best_effort" try our best to make private copies,
		 * but if we fail just carry on with the part of the action history
		 * that we can safely use (if any).
		 */
		actions_make_private_copy(buf, fd_obj, 1);
	}
	else if (strcmp(buf->options->undo_after_save, "never")) {
		/* Invalid option value. We shouldn't get here, but just in case... */
		err = EINVAL;
		goto fail1;
	}

	/* 
	 * Create the overlap graph and remove any cycles
	 */
	overlap_graph_t *g;
	err = create_overlap_graph(&g, buf->segcol, fd_obj);
	if (err)
		goto fail1;

	/* Remove cycles from the graph */
	err = overlap_graph_remove_cycles(g);
	if (err)
		goto fail2;

	/* Get a list of the edges that are not in the graph */
	struct list *removed_edges;
	err = overlap_graph_get_removed_edges(g, &removed_edges);
	if (err)
		goto fail2;

	/* Break each edge not in the graph */
	struct list_node *first_node = 
		list_head(removed_edges, struct edge_entry, ln)->next;
	struct list_node *node;

	list_for_each(first_node, node) {
		struct edge_entry *e = list_entry(node, struct edge_entry, ln);

		err = break_edge(buf->segcol, e, buf->options->tmp_dir);
		if (err)
			goto fail3;
	}

	list_free(removed_edges, struct edge_entry, ln);
	overlap_graph_free(g);

	/* 
	 * Create new segcol and put in the fd_obj. We do this here (instead of
	 * after having saved the data) so that any memory allocation errors
	 * don't leave the user with a changed file. This is in the spirit that
	 * when a function fails it should do its best to retain the state that
	 * existed before it was called.
	 */
	segcol_t *segcol_tmp;
	err = segcol_list_new(&segcol_tmp);
	if (err)
		goto fail1;

	segment_t *fd_seg;
	err = segment_new(&fd_seg, fd_obj, 0, segcol_size, data_object_update_usage);
	if (err)
		goto fail4;

	err = segcol_append(segcol_tmp, fd_seg);
	if (err) {
		segment_free(fd_seg);
		goto fail4;
	}

	/* 
	 * Create the overlap graph again and get the nodes in
	 * topological order.
	 */
	err = create_overlap_graph(&g, buf->segcol, fd_obj);
	if (err)
		goto fail4;

	/* Write the file segments to file in topological order */
	struct list *vertices;
	err = overlap_graph_get_vertices_topo(g, &vertices);
	if (err)
		goto fail5;

	first_node =
		list_head(vertices, struct vertex_entry, ln)->next;

	list_for_each(first_node, node) {
		struct vertex_entry *v = list_entry(node, struct vertex_entry, ln);
		err = write_segment(fd, v->segment, v->mapping, v->self_loop_weight);
		if (err)
			goto fail6;
	}
	
	list_free(vertices, struct vertex_entry, ln);
	overlap_graph_free(g);

	/* Write the rest of the segments */
	err = write_segcol_rest(fd, buf->segcol, fd_obj);
	if (err)
		goto fail4;

	/* Truncate file to final size (only if it is a resizable file) */
	if (fd_resizable == 1) {
		err = ftruncate(fd, segcol_size);
		if (err == -1) {
			err = errno;
			goto fail4;
		}
	}

	/* Use the new segcol in the buffer */
	segcol_free(buf->segcol);
	buf->segcol = segcol_tmp;

	if (!strcmp(buf->options->undo_after_save, "never")) {
		/* If the policy is "never" clear the undo/redo lists */
		action_list_clear(buf->undo_list);
		buf->undo_list_size = 0;

		action_list_clear(buf->redo_list);
		buf->redo_list_size = 0;
	}

	/* Call event callback if supplied by the user */
	if (buf->event_func != NULL) {
        struct bless_buffer_event_info event_info;
		event_info.event_type = BLESS_BUFFER_EVENT_SAVE;
        event_info.action_type = BLESS_BUFFER_ACTION_NONE;
        event_info.range_start = -1;
        event_info.range_length = -1;
        event_info.save_fd = fd;
		(*buf->event_func)(buf, &event_info, buf->event_user_data);
	}

	return 0;

/* Prevent memory leaks on failure */
fail3:
	list_free(removed_edges, struct edge_entry, ln);
fail2:
	overlap_graph_free(g);
fail1:
	data_object_free(fd_obj);

	return_error(err);

fail6:
	list_free(vertices, struct vertex_entry, ln);
fail5:
	overlap_graph_free(g);
fail4:
	segcol_free(segcol_tmp);
	goto fail1;

}

/**
 * Frees a bless_buffer_t.
 *
 * Freeing a bless_buffer_t frees all related resources.
 * 
 * @param buf the bless_buffer_t to close
 * @return the operation error code
 */
int bless_buffer_free(bless_buffer_t *buf)
{
	if (buf == NULL)
		return_error(EINVAL);

	int err = segcol_free(buf->segcol);
	if (err)
		return_error(err);

	err = buffer_options_free(buf->options);
	if (err)
		return_error(err);

	/* Free the stored undo actions */
	struct list_node *node;

	list_for_each(action_list_head(buf->undo_list)->next, node) {
		struct buffer_action_entry *entry =
			list_entry(node, struct buffer_action_entry , ln);

		buffer_action_free(entry->action);
	}

	list_free(buf->undo_list, struct buffer_action_entry, ln);

	/* Free the stored redo actions */
	list_for_each(action_list_head(buf->redo_list)->next, node) {
		struct buffer_action_entry *entry =
			list_entry(node, struct buffer_action_entry , ln);

		buffer_action_free(entry->action);
	}

	list_free(buf->redo_list, struct buffer_action_entry, ln);

	free(buf);

	return 0;
}

#pragma GCC visibility pop

