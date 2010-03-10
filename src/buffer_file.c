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
#include "debug.h"
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
	 * If the segment overlaps with itself and has moved to a higher address we
	 * must write it in a safe way (starting from the end). This is necessary
	 * in order to avoid overwriting data in the file that we need for later
	 * parts of the segment.
	 */
	if (overlap > 0 && mapping >= seg_start) {
		/* if the segment has not moved at all, don't write anything */
		if (mapping == seg_start)
			return 0;

		err = write_data_object_safe(dobj, seg_start, nwrite, fd, mapping);
		if (err)
			return_error(err);
	}
	else {
		err = write_data_object(dobj, seg_start, nwrite, fd, mapping);
		if (err)
			return_error(err);
	}

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
			if (err)
				goto_error(err, out);
		}

		segcol_iter_next(iter);
	}

out:
	segcol_iter_free(iter);

	return err;
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
	list_for_each_reverse_safe(list_tail(buf->undo_list)->prev, node, tmp) {
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
	list_for_each_safe(list_head(buf->redo_list)->next, node, tmp) {
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
	int err = 0;

	struct buffer_options *o = malloc(sizeof(**opts));
	if (o == NULL)
		return_error(ENOMEM);

	/* Set default values for options */
	o->tmp_dir = strdup("/tmp");
	if (o->tmp_dir == NULL) {
		err = ENOMEM;
		goto_error(err, on_error_mem_tmp_dir);
	}

	o->undo_limit = __MAX(size_t);

	o->undo_limit_str = strdup("infinite");
	if (o->undo_limit_str == NULL) {
		err = ENOMEM;
		goto_error(err, on_error_mem_undo_limit_str);
	}

	o->undo_after_save = strdup("best_effort");
	if (o->undo_after_save == NULL)	{
		err = ENOMEM;
		goto_error(err, on_error_mem_undo_after_save);
	}

	*opts = o;

	return 0;

on_error_mem_undo_after_save:
	free(o->undo_limit_str);
on_error_mem_undo_limit_str:
	free(o->tmp_dir);
on_error_mem_tmp_dir:
	free(o);
	return err;
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

/** 
 * Frees a vertex list and its contents.
 * 
 * @param list the list to free
 */
static void free_vertex_list(list_t *list)
{
	struct list_node *node;
	struct list_node *tmp;

	list_for_each_safe(list_head(list)->next, node, tmp) {
		struct vertex_entry *entry =
			list_entry(node, struct vertex_entry, ln);

		free(entry);
	}

	list_free(list);
}

/** 
 * Frees an edge list and its contents.
 * 
 * @param list the list to free
 */
static void free_edge_list(list_t *list)
{
	struct list_node *node;
	struct list_node *tmp;

	list_for_each_safe(list_head(list)->next, node, tmp) {
		struct edge_entry *entry =
			list_entry(node, struct edge_entry, ln);

		free(entry);
	}

	list_free(list);
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
		goto_error(err, on_error_segcol);

	err = buffer_options_new(&(*buf)->options);
	if (err)
		goto_error(err, on_error_options);
		
	err = list_new(&(*buf)->undo_list, struct buffer_action_entry, ln);
	if (err)
		goto_error(err, on_error_undo);

	(*buf)->undo_list_size = 0;

	err = list_new(&(*buf)->redo_list, struct buffer_action_entry, ln);
	if (err)
		goto_error(err, on_error_redo);

	(*buf)->redo_list_size = 0;
	(*buf)->multi_action = NULL;
	(*buf)->multi_action_mode = 0;
	(*buf)->event_func = NULL;
	(*buf)->event_user_data = NULL;

	return 0;

	/* Handle errors */
on_error_redo:
	list_free((*buf)->undo_list);
on_error_undo:
	buffer_options_free((*buf)->options);
on_error_options:
	segcol_free((*buf)->segcol);
on_error_segcol:
	free(buf);

	return err;
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
	UNUSED_PARAM(progress_func);

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
			goto_error(err, on_error_1);
	}
	else if (!strcmp(buf->options->undo_after_save, "best_effort")) {
		/* 
		 * If the policy is "best_effort" try our best to make private copies,
		 * but if we on_error_ just carry on with the part of the action history
		 * that we can safely use (if any).
		 */
		actions_make_private_copy(buf, fd_obj, 1);
	}
	else if (strcmp(buf->options->undo_after_save, "never")) {
		/* Invalid option value. We shouldn't get here, but just in case... */
		err = EINVAL;
		if (err)
			goto_error(err, on_error_1);
	}

	/* 
	 * Create the overlap graph and remove any cycles
	 */
	overlap_graph_t *g;
	err = create_overlap_graph(&g, buf->segcol, fd_obj);
	if (err)
		goto_error(err, on_error_1);

	/* Remove cycles from the graph */
	err = overlap_graph_remove_cycles(g);
	if (err)
		goto_error(err, on_error_2);

	/* Get a list of the edges that are not in the graph */
	list_t *removed_edges;
	err = overlap_graph_get_removed_edges(g, &removed_edges);
	if (err)
		goto_error(err, on_error_2);

	/* Break each edge not in the graph */
	struct list_node *first_node = 
		list_head(removed_edges)->next;
	struct list_node *node;

	list_for_each(first_node, node) {
		struct edge_entry *e = list_entry(node, struct edge_entry, ln);

		err = break_edge(buf->segcol, e, buf->options->tmp_dir);
		if (err)
			goto_error(err, on_error_3);
	}

	free_edge_list(removed_edges);
	overlap_graph_free(g);

	/* 
	 * Create new segcol and put in the fd_obj. We do this here (instead of
	 * after having saved the data) so that any memory allocation errors
	 * don't leave the user with a changed file. This is in the spirit that
	 * when a function on_error_s it should do its best to retain the state that
	 * existed before it was called.
	 */
	segcol_t *segcol_tmp;
	err = segcol_list_new(&segcol_tmp);
	if (err)
		goto_error(err, on_error_1);

	segment_t *fd_seg;
	err = segment_new(&fd_seg, fd_obj, 0, segcol_size, data_object_update_usage);
	if (err)
		goto_error(err, on_error_4);

	err = segcol_append(segcol_tmp, fd_seg);
	if (err) {
		segment_free(fd_seg);
		if (err)
			goto_error(err, on_error_4);
	}

	/* 
	 * Create the overlap graph again and get the nodes in
	 * topological order.
	 */
	err = create_overlap_graph(&g, buf->segcol, fd_obj);
	if (err)
		goto_error(err, on_error_4);

	/* Write the file segments to file in topological order */
	list_t *vertices;
	err = overlap_graph_get_vertices_topo(g, &vertices);
	if (err)
		goto_error(err, on_error_5);

	first_node =
		list_head(vertices)->next;

	list_for_each(first_node, node) {
		struct vertex_entry *v = list_entry(node, struct vertex_entry, ln);
		err = write_segment(fd, v->segment, v->mapping, v->self_loop_weight);
		if (err)
			goto_error(err, on_error_6);
	}
	
	free_vertex_list(vertices);
	overlap_graph_free(g);

	/* Write the rest of the segments */
	err = write_segcol_rest(fd, buf->segcol, fd_obj);
	if (err)
		goto_error(err, on_error_4);

	/* Truncate file to final size (only if it is a resizable file) */
	if (fd_resizable == 1) {
		err = ftruncate(fd, segcol_size);
		if (err == -1) {
			err = errno;
			goto_error(err, on_error_4);
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

/* Prevent memory leaks on on_error_ure */
on_error_3:
	free_edge_list(removed_edges);
on_error_2:
	overlap_graph_free(g);
on_error_1:
	data_object_free(fd_obj);

	return err;

on_error_6:
	free_vertex_list(vertices);
on_error_5:
	overlap_graph_free(g);
on_error_4:
	segcol_free(segcol_tmp);
	goto on_error_1;

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
	struct list_node *tmp;

	list_for_each_safe(list_head(buf->undo_list)->next, node, tmp) {
		struct buffer_action_entry *entry =
			list_entry(node, struct buffer_action_entry , ln);

		buffer_action_free(entry->action);
		free(entry);
	}

	list_free(buf->undo_list);

	/* Free the stored redo actions */
	list_for_each_safe(list_head(buf->redo_list)->next, node, tmp) {
		struct buffer_action_entry *entry =
			list_entry(node, struct buffer_action_entry , ln);

		buffer_action_free(entry->action);
		free(entry);
	}

	list_free(buf->redo_list);

	free(buf);

	return 0;
}

#pragma GCC visibility pop

