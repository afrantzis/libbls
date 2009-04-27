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
 * @file buffer_util.c
 *
 * Implementation of utility function used by bless_buffer_t
 */

#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "buffer.h"
#include "buffer_util.h"
#include "buffer_internal.h"
#include "segcol.h"
#include "segment.h"
#include "data_object.h"
#include "data_object_memory.h"
#include "data_object_file.h"
#include "util.h"

#include "type_limits.h"


/**
 * Reads data from a data object to memory.
 *
 * @param dobj the data object to read from
 * @param offset the offset in the data object to read from
 * @param mem the memory to save the data to
 * @param length the number of bytes to read
 *
 * @return the operation error code
 */
int read_data_object(data_object_t *dobj, off_t offset, void *mem, off_t length)
{
	unsigned char *cur_dst = mem;

	while (length > 0) {
		void *data;
		off_t nbytes = length;
		int err = data_object_get_data(dobj, &data, offset, &nbytes,
				DATA_OBJECT_READ);
		if (err)
			return_error(err);

		/* Copy data to provided buffer */
		/* 
		 * Note that the length of data returned by data_object_get_data is
		 * guaranteed to fit in a ssize_t, hence the cast is safe. 
		 */
		memcpy(cur_dst, data, (ssize_t)nbytes);

		/* 
		 * cur_dst and offset may overflow here in the last iteration. This
		 * doesn't affect the algorithm because their values won't be used
		 * (because it's the last iteration). The problem is that signed
		 * overflow leads to undefined behaviour according to the C standard,
		 * so we must not allow offset to overflow. Unsigned overflow just
		 * wraps around so there is no problem for cur_dst.
		 */
		if (__MAX(off_t) - offset >= nbytes)
			offset += nbytes;
		cur_dst += nbytes;
		length -= nbytes;
	}

	return 0;
}

/**
 * Writes data from a data object to a file.
 *
 * @param dobj the data object to read from
 * @param offset the offset in the data object to read from
 * @param length the number of bytes to read
 * @param fd the file descriptor to write the data to
 * @param file_offset the offset in the file to write the data
 *
 * @return the operation error code
 */
int write_data_object(data_object_t *dobj, off_t offset, off_t length,
		int fd, off_t file_offset)
{
	off_t s = lseek(fd, file_offset, SEEK_SET);
	if (s != file_offset)
		return_error(errno);

	while (length > 0) {
		void *data;
		off_t nbytes = length;
		int err = data_object_get_data(dobj, &data, offset, &nbytes,
				DATA_OBJECT_READ);
		if (err)
			return_error(err);

		/* 
		 * Note that the length of data returned by data_object_get_data is
		 * guaranteed to fit in a ssize_t, hence the casts are safe.
		 */
		ssize_t nwritten = write(fd, data, (ssize_t)nbytes);
		if (nwritten < (ssize_t)nbytes)
			return_error(errno);

		/* See read_data_object() about this check */
		if (__MAX(off_t) - offset >= nbytes)
			offset += nbytes;
		length -= nbytes;
	}

	return 0;
}

/**
 * Gets from an iterator the read limits.
 *
 * @param iter the iterator to get data from
 * @param[out] segment the segment_t pointed to by the iterator 
 * @param[out] mapping the mapping of the segment in its segcol
 * @param[out] read_start the offset in the segment's data object to start
 *             reading from
 * @param[out] read_length the length of the data we should read
 * @param offset the offset in the segcol we want to read from
 * @param length the length of the data we want to read
 *
 * @return the operation error code
 */
static int get_data_from_iter(segcol_iter_t *iter, segment_t **segment,
		off_t *mapping, off_t *read_start, off_t *read_length, off_t offset,
		off_t length)
{
	int err = segcol_iter_get_segment(iter, segment);
	if (err)
		return_error(err);

	err = segcol_iter_get_mapping(iter, mapping);
	if (err)
		return_error(err);

	off_t seg_start;
	err = segment_get_start(*segment, &seg_start);
	if (err)
		return_error(err);

	off_t seg_size;
	err = segment_get_size(*segment, &seg_size);
	if (err)
		return_error(err);

	if (length == 0 || seg_size == 0) {
		*read_length = 0;
		return 0;
	}

	/* The index of the start of the logical range in the segment range */
	off_t start_index = offset - *mapping;

	/* Check overflow */
	if (__MAX(off_t) - offset < length - 1 * (length != 0))
		return_error(EOVERFLOW);

	/* The index of the end of the logical range in the segment range */
	off_t end_index = (offset + length - 1 * (length != 0)) - *mapping;

	/* 
	 * If the end of the logical range is outside the segment range,
	 * clip the logical range to the end of the segment range.
	 */
	if (end_index >= seg_size)
		end_index = seg_size - 1;

	if (__MAX(off_t) - seg_start < start_index)
		return_error(EOVERFLOW);

	/* Calculate the read range for the data object */
	*read_start = seg_start + start_index;

	/* 
	 * read_length cannot overflow because 
	 * end_index - start_index <= length - 1 <= MAX_OFF_T - 1
	 */
	*read_length = end_index - start_index + 1;

	return 0;
}

/**
 * Calls a function for each segment in the specified range of a segcol_t.
 *
 * @param segcol the segcol_t to search in
 * @param offset the offset in the segcol_t to start from
 * @param length the length of the range
 * @param func the function to call for each segment
 * @param user_data user specified data to pass to func
 *
 * @return the operation error code
 */
int segcol_foreach(segcol_t *segcol, off_t offset, off_t length,
		segcol_foreach_func *func, void *user_data)
{
	if (segcol == NULL || offset < 0 || length < 0 || func == NULL)
		return_error(EINVAL);

	/* Check for overflow */
	if (__MAX(off_t) - offset < length - 1 * (length != 0))
		return_error(EOVERFLOW);

	/* Make sure that the range is valid */
	off_t segcol_size;
	segcol_get_size(segcol, &segcol_size);

	if (offset + length - 1 * (length != 0) >= segcol_size)
		return_error(EINVAL);

	/* Get iterator to offset */
	segcol_iter_t *iter;
	int err = segcol_find(segcol, &iter, offset);
	if (err)
		return_error(err);

	off_t cur_offset = offset;

	/* How many bytes we still have to read */
	off_t bytes_left = length;

	int iter_valid;

	/* 
	 * Iterate over the given range 
	 */
	while (!(err = segcol_iter_is_valid(iter, &iter_valid)) && iter_valid) {
		/* Get necessary data from the iterator */
		segment_t *segment;
		off_t mapping;
		off_t read_start;
		off_t read_length;

		err = get_data_from_iter(iter, &segment, &mapping, &read_start, 
				&read_length, cur_offset, bytes_left);
		if (err)
			goto fail;

		/* Call user provided function */
		err = (*func)(segcol, segment, mapping, read_start, read_length,
				user_data);
		if (err)
			goto fail;

		bytes_left -= read_length;

		/* Don't allow signed overflow. See read_data_object() for more. */
		if (__MAX(off_t) - cur_offset >= read_length)
			cur_offset += read_length;

		if (bytes_left == 0)
			break;

		/* Move to next segment */
		err = segcol_iter_next(iter);
		if (err)
			goto fail;
	}

	segcol_iter_free(iter);
	return 0;

fail:
	segcol_iter_free(iter);
	return_error(err);

}


/**
 * A segcol_foreach_func that reads data from a segment_t into memory.
 *
 * @param segcol the segcol_t containing the segment
 * @param seg the segment to read from
 * @param mapping the mapping of the segment in segcol
 * @param read_start the offset in the data of the segment to start reading
 * @param read_length the length of the data to read
 * @param user_data a void ** pointer containing the pointer to write to
 *
 * @return the operation error code
 */
static int read_segment_func(segcol_t *segcol, segment_t *seg,
		off_t mapping, off_t read_start, off_t read_length, void *user_data)
{
	data_object_t *dobj;
	segment_get_data(seg, (void **)&dobj);

	/* user_data is actually a pointer to pointer*/
	unsigned char **dst = (unsigned char **)user_data;

	int err = read_data_object(dobj, read_start, *dst, read_length);
	if (err)
		return_error(err);

	/* Move the pointer forwards */
	*dst += read_length;

	return 0;
}

/**
 * Stores a range from a segcol_t in memory data objects.
 *
 * @param segcol the segcol_t 
 * @param offset the offset to starting storing from
 * @param length the length of the data to store
 *
 * @return the operation error code 
 */
int segcol_store_in_memory(segcol_t *segcol, off_t offset, off_t length)
{
	if (segcol == NULL || offset < 0 || length < 0)
		return_error(EINVAL);

	/* Create data object to hold data */
	void *new_data = malloc(length);
	if (new_data == NULL)
		return_error(ENOMEM);

	data_object_t *new_dobj;
	int err = data_object_memory_new(&new_dobj, new_data, length);
	if (err) {
		free(new_data);
		return_error(err);
	}

	/* Set the data object's free function */
	err = data_object_memory_set_free_func(new_dobj, free);
	if (err) {
		free(new_data);
		data_object_free(new_dobj);
		return_error(err);
	}

	/* Put it in a segment */
	segment_t *new_seg;
	err = segment_new(&new_seg, new_dobj, 0, length, data_object_update_usage);
	if (err) {
		data_object_free(new_dobj);
		return_error(err);
	}

	void *data_ptr = new_data;

	/* Store data from range in memory pointed by new_data */
	err = segcol_foreach(segcol, offset, length, read_segment_func, &data_ptr);
	if (err) {
		segment_free(new_seg);
		return_error(err);
	}

	/* Delete old range in segcol */
	err = segcol_delete(segcol, NULL, offset, length);
	if (err) {
		segment_free(new_seg);
		return_error(err);
	}

	off_t segcol_size;
	segcol_get_size(segcol, &segcol_size);

	/* 
	 * Add to segcol the new segment that hold the same data as the deleted
	 * range (but keeps them in memory)
	 */
	if (offset < segcol_size)
		err = segcol_insert(segcol, offset, new_seg);
	else if (offset == segcol_size)
		err = segcol_append(segcol, new_seg);

	/* TODO: Handle this better because the segcol is going to be corrupted */
	if (err)
		return_error(err);

	return 0;
}

/**
 * A segcol_foreach_func that writes data from a segment_t into a file.
 *
 * @param segcol the segcol_t containing the segment
 * @param seg the segment to read from
 * @param mapping the mapping of the segment in segcol
 * @param read_start the offset in the data of the segment to start reading
 * @param read_length the length of the data to read
 * @param user_data a int * pointer pointing to the fd to write to
 *
 * @return the operation error code
 */
static int store_segment_func(segcol_t *segcol, segment_t *seg,
		off_t mapping, off_t read_start, off_t read_length, void *user_data)
{
	data_object_t *dobj;
	segment_get_data(seg, (void **)&dobj);

	/* user_data is actually a pointer to a file descriptor */
	int fd = *(int *)user_data;

	off_t cur_off = lseek(fd, 0, SEEK_CUR);

	int err = write_data_object(dobj, read_start, read_length, fd, cur_off);
	if (err)
		return_error(err);

	return 0;
}

/**
 * Stores a range from a segcol_t in a file data object.
 *
 * @param segcol the segcol_t 
 * @param offset the offset to starting storing from
 * @param length the length of the data to store
 * @param tmpdir the directory where to create the file
 *
 * @return the operation error code 
 */

int segcol_store_in_file(segcol_t *segcol, off_t offset, off_t length,
		char *tmpdir)
{
	if (segcol == NULL || offset < 0 || length < 0 || tmpdir == NULL)
		return_error(EINVAL);

	/* Create temporary path string */
	char *file_tmpl = "lb-XXXXXX";
	char *tmpl = NULL;

	int err = path_join(&tmpl, tmpdir, file_tmpl);
	if (err)
		return err;

	/* Store data from range to temporary file */
	int fd = mkstemp(tmpl);
	if (fd == -1) {
		free(tmpl);
		return_error(errno);
	}

	int *fd_ptr = &fd;

	err = segcol_foreach(segcol, offset, length, store_segment_func,
			fd_ptr);
	if (err) {
		close(fd);
		unlink(tmpl);
		free(tmpl);
		return_error(err);
	}

	/* 
	 * Create file data object with temporary file.
	 * Note that this must be done after we have filled the file with the
	 * data, because the data object's size is set only once when creating it.
	 * (so if we did this before the data object would have a size of 0)
	 */
	data_object_t *new_dobj;
	err = data_object_tempfile_new(&new_dobj, fd, tmpl);
	if (err) {
		close(fd);
		unlink(tmpl);
		free(tmpl);
		return_error(err);
	}

	/* We don't need tmpl any longer */
	free(tmpl);

	/* Set the data object's close function */
	err = data_object_file_set_close_func(new_dobj, close);
	if (err) {
		close(fd);
		data_object_free(new_dobj);
		return_error(err);
	}

	/* Put it in a segment */
	segment_t *new_seg;
	err = segment_new(&new_seg, new_dobj, 0, length, data_object_update_usage);
	if (err) {
		data_object_free(new_dobj);
		return_error(err);
	}

	/* Delete old range in segcol */
	err = segcol_delete(segcol, NULL, offset, length);
	if (err) {
		segment_free(new_seg);
		return_error(err);
	}

	off_t segcol_size;
	segcol_get_size(segcol, &segcol_size);

	/* 
	 * Add to segcol the new segment that hold the same data as the deleted
	 * range (but keeps them in memory)
	 */
	if (offset < segcol_size)
		err = segcol_insert(segcol, offset, new_seg);
	else if (offset == segcol_size)
		err = segcol_append(segcol, new_seg);

	/* TODO: Handle this better because the segcol is going to be corrupted */
	if (err)
		return_error(err);

	return 0;
}

/** 
 * Copies data from a segcol into another.
 * 
 * The dst and src segcol must not be the same.
 *
 * @param dst the segcol to copy data into
 * @param offset the offset in the segcol to copy data into
 * @param src the segcol to copy data from
 * 
 * @return the operation error code
 */
int segcol_add_copy(segcol_t *dst, off_t offset, segcol_t *src)
{
	if (dst == NULL || src == NULL || offset < 0 || dst == src)
		return_error(EINVAL);

	off_t dst_size;
	int err = segcol_get_size(dst, &dst_size);
	if (err)
		return_error(err);

	segcol_iter_t *iter;
	err = segcol_iter_new(src, &iter);
	if (err)
		return_error(err);

	/* If the deleted data was beyond the end of file we must append it */
	int use_append = (offset >= dst_size);

	/* The offset of the last byte we re-added to the segcol */
	off_t offset_reached = offset - 1;

	int valid;

	/* Re-add a copy of every segment to the segcol at its original position */
	while (!segcol_iter_is_valid(iter, &valid) && valid) {
		segment_t *seg;
		off_t mapping;
		segcol_iter_get_segment(iter, &seg);
		segcol_iter_get_mapping(iter, &mapping);

		segment_t *seg_copy;
		segment_copy(seg, &seg_copy);

		if (use_append)
			err = segcol_append(dst, seg_copy);
		else
			err = segcol_insert(dst, offset + mapping, seg_copy);

		if (err) {
			segment_free(seg_copy);
			goto fail;
		}

		offset_reached = offset + mapping - 1;

		err = segcol_iter_next(iter);
		if (err)
			goto fail;
	}

	err = segcol_iter_free(iter);
	if (err)
		goto fail_iter_free;

	return 0;

fail:
	segcol_iter_free(iter);
fail_iter_free:
	/* 
	 * If we fail try to restore the previous state of the buffer by
	 * deleting any segments we re-added.
	 */
	if (offset_reached >= offset)
		segcol_delete(dst, NULL, offset, offset_reached - offset + 1);

	return_error(err);
}

/**
 * Enforces the undo limit on the undo list.
 *
 * After the operation the undo list contains at most the most recent
 * buf->options->undo_limit actions. Additionally if ensure_vacancy == 1 the
 * undo list contains space for at least one action (unless the undo limit is
 * 0).
 * 
 * @param buf the bless_buffer_t
 * @param ensure_vacancy whether to make sure that there is space for one
 *                       additional action
 * 
 * @return the operation error code
 */
int undo_list_enforce_limit(bless_buffer_t *buf, int ensure_vacancy)
{
	if (buf == NULL)
		return_error(EINVAL);

	size_t limit = buf->options->undo_limit;
	/* 
	 * if we want to ensure vacancy of one action we must delete one more
	 * existing action than we would normally do.
	 */
	if (limit != 0 && ensure_vacancy)
		limit--;

	/* 
	 * Remove actions from the start of the undo list (older ones) until
	 * we reach the limit.
	 */
	struct list_node *node;
	struct list_node *tmp;

  list_for_each_safe(action_list_head(buf->undo_list)->next, node, tmp) {
    if (buf->undo_list_size <= limit)
      break;

		int err = list_delete_chain(node, node);
		if (err)
			return_error(err);

    --buf->undo_list_size;

		struct buffer_action_entry *del_entry = 
			list_entry(node, struct buffer_action_entry, ln);

		buffer_action_free(del_entry->action);
		free(del_entry);
  }


	return 0;
}

/** 
 * Clears an action list's contents without freeing the list itself.
 * 
 * @param action_list the action_list
 * 
 * @return the operation error code
 */
int action_list_clear(struct list *action_list)
{
	if (action_list == NULL)
		return_error(EINVAL);

	struct list_node *node;
	struct list_node *tmp;

	/* 
	 * Use the safe iterator so that we can delete the current 
	 * node from the list as we traverse it.
	 */
	list_for_each_safe(action_list_head(action_list)->next, node, tmp) {
		struct buffer_action_entry *entry =
			list_entry(node, struct buffer_action_entry , ln);

		list_delete_chain(node, node);
		buffer_action_free(entry->action);
		free(entry);
	}

	return 0;
}

