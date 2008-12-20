/**
 * @file buffer_util.c
 *
 * Implementation of utility function used by bless_buffer_t
 */

#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "buffer_util.h"
#include "segcol.h"
#include "segment.h"
#include "data_object.h"
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
		size_t nbytes = length;
		int err = data_object_get_data(dobj, &data, offset, &nbytes,
				DATA_OBJECT_READ);
		if (err)
			return err;

		/* Copy data to provided buffer */
		memcpy(cur_dst, data, nbytes);

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
	off_t s = lseek(fd, file_offset);
	if (s != file_offset)
		return errno;

	while (length > 0) {
		void *data;
		size_t nbytes = length;
		int err = data_object_get_data(dobj, &data, offset, &nbytes,
				DATA_OBJECT_READ);
		if (err)
			return err;

		err = write(fd, data, nbytes);
		if (err == -1)
			return errno;

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
		return err;

	err = segcol_iter_get_mapping(iter, mapping);
	if (err)
		return err;

	off_t seg_start;
	err = segment_get_start(*segment, &seg_start);
	if (err)
		return err;

	off_t seg_size;
	err = segment_get_size(*segment, &seg_size);
	if (err)
		return err;

	if (length == 0 || seg_size == 0) {
		*read_length = 0;
		return 0;
	}

	/* The index of the start of the logical range in the segment range */
	off_t start_index = offset - *mapping;

	/* Check overflow */
	if (__MAX(off_t) - offset < length - 1 * (length != 0))
		return EOVERFLOW;

	/* The index of the end of the logical range in the segment range */
	off_t end_index = (offset + length - 1 * (length != 0)) - *mapping;

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
		return EINVAL;

	/* Check for overflow */
	if (__MAX(off_t) - offset < length - 1 * (length != 0))
		return EOVERFLOW;

	/* Make sure that the range is valid */
	off_t segcol_size;
	segcol_get_size(segcol, &segcol_size);

	if (offset + length - 1 * (length != 0) >= segcol_size)
		return EINVAL;

	/* Get iterator to offset */
	segcol_iter_t *iter;
	int err = segcol_find(segcol, &iter, offset);
	if (err)
		return err;

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

	err = 0;

fail:
	segcol_iter_free(iter);
	return err;

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

	/* user_data is actually a void ** pointer */
	void **dst = (void **)user_data;

	int err = read_data_object(dobj, read_start, *dst, read_length);
	if (err)
		return err;

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
		return EINVAL;

	/* Create data object to hold data */
	void *new_data = malloc(length);
	if (new_data == NULL)
		return ENOMEM;

	data_object_t *new_dobj;
	int err = data_object_memory_new(&new_dobj, new_data, length);
	if (err) {
		free(new_data);
		return err;
	}

	/* Put it in a segment */
	segment_t *new_seg;
	err = segment_new(&new_seg, new_dobj, 0, length, data_object_update_usage);
	if (err) {
		data_object_free(new_dobj);
		free(new_data);
		return err;
	}

	void *data_ptr = new_data;

	/* Store data from range in memory pointed by new_data */
	err = segcol_foreach(segcol, offset, length, read_segment_func, &data_ptr);
	if (err) {
		segment_free(new_seg);
		return err;
	}

	/* Delete old range in segcol */
	err = segcol_delete(segcol, NULL, offset, length);
	if (err) {
		segment_free(new_seg);
		return err;
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
		return err;

	return 0;
}
