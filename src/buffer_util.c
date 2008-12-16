/**
 * @file buffer_util.c
 *
 * Implementation of utility function used by bless_buffer_t
 */
#include "buffer_util.h"

#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include "segcol.h"
#include "segment.h"
#include "data_object.h"
#include "type_limits.h"


/**
 * Reads data from a data object.
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
 * Gets from an iterator the read limits.
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
 * Calls a function for each segment in the specified range.
 *
 * @param segcol
 *
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
	int err = segcol_get_size(segcol, &segcol_size);
	if (err)
		return err;

	if (offset + length - 1 * (length != 0) >= segcol_size)
		return EINVAL;

	/* Get iterator to offset */
	segcol_iter_t *iter;
	err = segcol_find(segcol, &iter, offset);
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

