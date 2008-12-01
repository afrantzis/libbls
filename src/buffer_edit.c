/**
 * @file buffer_edit.c
 *
 * Buffer edit operations
 *
 * @author Michael Iatrou
 * @author Alexandros Frantzis
 */

#include <errno.h>
#include <string.h>
#include "buffer.h"
#include "buffer_internal.h"
#include "data_object.h"
#include "data_object_memory.h"
#include "type_limits.h"


/********************/
/* Helper functions */
/********************/

/**
 * Get from an iterator the data object and read limits.
 *
 * @param iter the iterator to get data from
 * @param nata_obj the data object related to the segment in the iterator
 * @param read_start the offset in the data object to start reading from
 * @param read_length the length of the data we should read
 * @param offset the offset in the segcol we want to read from
 * @param length the length of the data we want to read
 *
 * @return the operation error code
 */
static int get_data_from_iter(segcol_iter_t *iter, data_object_t **data_obj,
		off_t *read_start, size_t *read_length, off_t offset, size_t length)
{
	segment_t *seg;
	int err = segcol_iter_get_segment(iter, &seg);
	if (err)
		return err;

	off_t mapping;
	err = segcol_iter_get_mapping(iter, &mapping);
	if (err)
		return err;

	off_t seg_start;
	err = segment_get_start(seg, &seg_start);
	if (err)
		return err;

	off_t seg_size;
	err = segment_get_size(seg, &seg_size);
	if (err)
		return err;

	err = segment_get_data(seg, (void **)data_obj);
	if (err)
		return err;

	if (length == 0 || seg_size == 0) {
		*read_length = 0;
		return 0;
	}

	/* The index of the start of the logical range in the segment range */
	off_t start_index = offset - mapping;

	/* Check overflow */
	if (__MAX(off_t) - offset < length - 1 * (length != 0))
		return EOVERFLOW;

	/* The index of the end of the logical range in the segment range */
	off_t end_index = (offset + length - 1 * (length != 0)) - mapping;

	/* 
	 * If the end of the logical range is outside the segment range,
	 * clip the logical range to the end of the segment range.
	 */
	if (end_index >= seg_size)
		end_index = seg_size - 1;

	if (__MAX(off_t) - seg_start < start_index)
		return EOVERFLOW;

	/* Calculate the read range for the data object */
	*read_start = seg_start + start_index;
	/* 
	 * read_length cannot overflow because 
	 * end_index - start_index <= length - 1 <= MAX_SIZE_T - 1
	 */
	*read_length = (size_t) (end_index - start_index + 1);

	return 0;
}

/*****************/
/* API Functions */
/*****************/

/**
 * Appends data to a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to append data to
 * @param data a pointer to the data to append
 * @param len the length of the data to append
 *
 * @return the operation error code
 */

int bless_buffer_append(bless_buffer_t *buf, void *data, size_t length)
{
	if (buf == NULL || data == NULL) 
		return EINVAL;

	/* 
	 * No need to check for overflow, because it is detected by the
	 * functions that follow.
	 */

	segcol_t *sc = buf->segcol;

	/* Create a data object and a segment pointing to it */
	data_object_t *obj;
	int err = data_object_memory_new_data(&obj, data, length);
	if (err)
		return err;

	segment_t *seg;
	err = segment_new(&seg, obj, 0, length, data_object_update_usage);
	if (err) {
		data_object_free(obj);
		return err;
	}

	/* Append segment to the segcol */
	err = segcol_append(sc, seg);
	if (err) 
		goto fail;

	/* 
	 * Give the ownership of the data to the data object.
	 * This must be done last. If it is done earlier and any function
	 * fails, the data_object_t will be freed and with it the
	 * data it contains. Alas, we don't want this to happen: if the
	 * function fails the caller will surely be expecting their data to
	 * still be available. 
	 */
	err = data_object_set_data_ownership(obj, 1);
	if (err)
		goto fail;

	return 0;
fail:
	/* No need to free obj, this is handled by segment_free */
	segment_free(seg);
	return err;
}

/**
 * Inserts data into a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to insert data into
 * @param offset the offset in the bless_buffer_t to insert data into
 * @param data a pointer to the data to insert
 * @param length the length of the data to insert
 *
 * @return the operation error code
 */
int bless_buffer_insert(bless_buffer_t *buf, off_t offset,
		void *data, size_t length)
{
	if (buf == NULL || data == NULL || offset < 0) 
		return EINVAL;

	/* 
	 * No need to check for overflow, because it is detected by the
	 * functions that follow.
	 */

	segcol_t *sc = buf->segcol;

	/* Create a data object and a segment pointing to it */
	data_object_t *obj;
	int err = data_object_memory_new_data(&obj, data, length);
	if (err)
		return err;

	segment_t *seg;
	err = segment_new(&seg, obj, 0, length, data_object_update_usage);
	if (err) {
		data_object_free(obj);
		return err;
	}

	/* Insert segment into the segcol */
	err = segcol_insert(sc, offset, seg);
	if (err)
		goto fail;

	/* 
	 * Give the ownership of the data to the data object.
	 * This must be done last. If it is done earlier and any function
	 * fails, the data_object_t will be freed and with it the
	 * data it contains. Alas, we don't want this to happen: if the
	 * function fails the caller will surely be expecting their data to
	 * still be available. 
	 */
	err = data_object_set_data_ownership(obj, 1);
	if (err)
		goto fail;

	return 0;

fail:
	/* No need to free obj, this is handled by segment_free */
	segment_free(seg);
	return err;
}

/**
 * Delete data from a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to delete data from
 * @param offset the offset in the bless_buffer_t to delete data from
 * @param length the length of the data to delete
 *
 * @return the operation error code
 */
int bless_buffer_delete(bless_buffer_t *buf, off_t offset, off_t length)
{
	return -1;
}

/**
 * Reads data from a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to read data from
 * @param src_offset the offset in the bless_buffer_t to start reading from
 * @param dst a pointer to the start of the memory to store the data into
 * @param dst_offset the offset in dst to the start storing data into
 * @param length the length of the data to read
 *
 * @return the operation error code
 */
int bless_buffer_read(bless_buffer_t *buf, off_t src_offset, void *dst,
		size_t dst_offset, size_t length)
{
	if (buf == NULL || src_offset < 0 || dst == NULL) 
		return EINVAL;

	/* Check for overflow */
	if (__MAX(off_t) - src_offset < length - 1 * (length != 0))
		return EOVERFLOW;

	if (__MAX(size_t) - (size_t)dst < dst_offset)
		return EOVERFLOW;

	if (__MAX(size_t) - (size_t)dst - dst_offset < length - 1 * (length != 0))
		return EOVERFLOW;

	/* Make sure that the range is valid */
	off_t buf_size;
	int err = segcol_get_size(buf->segcol, &buf_size);
	if (err)
		return err;

	if (src_offset + length - 1 * (length != 0) >= buf_size)
		return EINVAL;

	/* Get iterator to src_offset */
	segcol_iter_t *iter;
	err = segcol_find(buf->segcol, &iter, src_offset);
	if (err)
		return err;

	off_t cur_offset = src_offset;
	unsigned char *cur_dst = (unsigned char *)dst + dst_offset;

	/* How many bytes we still have to read */
	size_t bytes_left = length;

	int iter_valid;

	/* 
	 * Read the data by using the iterator to get all the segments
	 * (and related data objects) that correspond to the given range.
	 */
	while (!(err = segcol_iter_is_valid(iter, &iter_valid)) && iter_valid) {
		/* 
		 * Get the data object pointed to by the segment in the iterator
		 * and find out the range of the data object that we have to read.
		 */
		data_object_t *data_obj;
		off_t read_start;
		size_t read_length;
		err = get_data_from_iter(iter, &data_obj, &read_start, &read_length,
				cur_offset, bytes_left);
		if (err)
			goto fail;

		/* 
		 * Read data from data object. The data object may need to be
		 * queried multiple times to get the whole data.
		 */
		while (read_length > 0) {
			void *data;
			size_t nbytes = read_length;
			err = data_object_get_data(data_obj, &data, read_start, &nbytes,
					DATA_OBJECT_READ);
			if (err)
				goto fail;

			/* Copy data to user-provided buffer */
			memcpy(cur_dst, data, nbytes);

			/* 
			 * cur_dst, read_start and cur_offset may overflow here in the last
			 * iteration. This doesn't affect the algorithm because their
			 * values won't be used (because it's the last iteration). The
			 * problem is that signed overflow leads to undefined behaviour
			 * according to the C standard, so will must not allow read_start 
			 * and cur_offset to overflow. Unsigned overflow just wraps around
			 * so there is no problem for cur_dst.
			 */
			cur_dst += nbytes;
			read_length -= nbytes;
			bytes_left -= nbytes;

			if (__MAX(off_t) - read_start >= nbytes)
				read_start += nbytes;
			if (__MAX(off_t) - cur_offset >= nbytes)
				cur_offset += nbytes;
		}

		if (bytes_left == 0)
			break;

		/* Move to next segment */
		err = segcol_iter_next(iter);
		if (err)
			goto fail;
	}

	err = 0;

fail:
	segcol_iter_free(iter);
	return err;
}

/**
 * Copies data from a bless_buffer_t to another.
 *
 * @param src the bless_buffer_t to read data from
 * @param src_offset the offset in the source bless_buffer_t to start reading from
 * @param dst the bless_buffer_t to copy data to
 * @param dst_offset the offset in the destination bless_buffer_t to start copying data to
 * @param length the length of the data to read
 *
 * @return the operation error code
 */
int bless_buffer_copy(bless_buffer_t *src, off_t src_offset, bless_buffer_t *dst,
		off_t dst_offset, off_t length)
{
	return -1;
}

/**
 * Searches for data in a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to search
 * @param[out] match the offset the data was found at or -1 if no match was found
 * @param start_offset the offset in the bless_buffer_t to start searching from
 * @param data a pointer to the data to search for
 * @param length the length of the data to search for
 * @param cb the bless_progress_cb to call to report the progress of the
 *           operation or NULL to disable reporting
 *
 * @return the offset in the bless_buffer_t where a match was found or
 *         an operation error code
 */
int bless_buffer_find(bless_buffer_t *buf, off_t *match, off_t start_offset, 
		void *data, size_t length, bless_progress_cb cb)
{
	return -1;
}
