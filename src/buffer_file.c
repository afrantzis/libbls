/**
 * @file buffer_file.c
 *
 * Buffer file operations
 *
 * @author Alexandros Frantzis
 * @author Michael Iatrou
 */

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "buffer.h"
#include "buffer_internal.h"
#include "segcol_list.h"
#include "data_object.h"
#include "data_object_file.h"
#include "overlap_graph.h"
#include "list.h"
#include "buffer_util.h"
#include "util.h"


#pragma GCC visibility push(default)

/********************/
/* Helper functions */
/********************/

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
 *
 * @return the operation error code
 */ 
static int break_edge(segcol_t *segcol, struct edge_entry *edge)
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

	int err = segcol_store_in_memory(segcol, overlap_offset,
			edge->weight);

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
			if (err)
				segcol_iter_free(iter);
				return_error(err);
		}

		segcol_iter_next(iter);
	}

	segcol_iter_free(iter);

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
	if (err) {
		free(buf);
		return_error(err);
	}

	return 0;
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

	int err = reserve_disk_space(fd, segcol_size);
	if (err)
		return_error(err);

	/* Create a data_object_t holding fd */
	data_object_t *fd_obj;
	err = data_object_file_new(&fd_obj, fd);
	if (err)
		return_error(err);

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
		err = break_edge(buf->segcol, e);
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
	if (err) {
		segcol_free(segcol_tmp);
		goto fail1;
	}

	err = segcol_append(segcol_tmp, fd_seg);
	if (err) {
		segcol_free(segcol_tmp);
		goto fail1;
	}

	/* 
	 * Create the overlap graph again and get the nodes in
	 * topological order.
	 */
	err = create_overlap_graph(&g, buf->segcol, fd_obj);
	if (err)
		goto fail1;

	/* Write the file segments to file in topological order */
	struct list *vertices;
	err = overlap_graph_get_vertices_topo(g, &vertices);
	if (err)
		goto fail2;

	first_node =
		list_head(vertices, struct vertex_entry, ln)->next;

	list_for_each(first_node, node) {
		struct vertex_entry *v = list_entry(node, struct vertex_entry, ln);
		err = write_segment(fd, v->segment, v->mapping, v->self_loop_weight);
		if (err)
			goto fail4;
	}
	
	list_free(vertices, struct vertex_entry, ln);
	overlap_graph_free(g);

	/* Write the rest of the segments */
	err = write_segcol_rest(fd, buf->segcol, fd_obj);
	if (err)
		goto fail1;

	/* Truncate file to final size */
	err = ftruncate(fd, segcol_size);
	if (err == -1) {
		err = errno;
		goto fail1;
	}

	/* Use the new segcol in the buffer */
	segcol_free(buf->segcol);
	buf->segcol = segcol_tmp;

	return 0;

/* Prevent memory leaks on failure */
fail3:
	list_free(removed_edges, struct edge_entry, ln);
fail2:
	overlap_graph_free(g);
fail1:
	data_object_free(fd_obj);

	return_error(err);

fail4:
	list_free(vertices, struct vertex_entry, ln);
	goto fail2;

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

	free(buf);

	return 0;
}

#pragma GCC visibility pop

