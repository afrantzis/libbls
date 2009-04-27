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
 * @file segment.c
 *
 * Segment implementation
 */
#include <stdlib.h>
#include <errno.h>
#include "segment.h"
#include "type_limits.h"
#include "util.h"

struct segment {
	void *data;
	off_t start;
	off_t size;
	segment_data_usage_func data_usage_func;
};

/**
 * Creates a new segment_t.
 *
 * @param[out] seg the created segment or NULL
 * @param data the data object this segment is associated with
 * @param start the start offset of the segment
 * @param size the size of the segment
 * @param data_usage_func the function to call to update the usage count of
 *        the data associated with this segment (may be NULL)
 *
 * @return the operation error code
 */
int segment_new(segment_t **seg, void *data, off_t start, off_t size,
		segment_data_usage_func data_usage_func)
{
	segment_t *segp = NULL;

	segp = (segment_t *) malloc(sizeof(segment_t));

	if (segp == NULL)
		return_error(ENOMEM);

	/* Initialize to NULL, so that segment_set_data works correctly */
	segp->data_usage_func = NULL;

	int err = segment_set_data(segp, data, data_usage_func);
	if (err)
		goto fail;

	err = segment_set_range(segp, start, size);
	if (err)
		goto fail;

	*seg = segp;

	return 0;

fail:
	free(segp);
	return_error(err);
}

/**
 * Creates a copy of a segment_t.
 *
 * @param seg the segment_t to copy
 * @param[out] seg_copy the new copy of the segment
 *
 * @return the operation error code
 */
int segment_copy(segment_t *seg, segment_t **seg_copy)
{
	if (seg == NULL || seg_copy == NULL)
		return_error(EINVAL);

	int err = segment_new(seg_copy, seg->data, seg->start, seg->size,
			seg->data_usage_func);

	if (err)
		return_error(err);

	return 0;
}

/**
 * Frees a segment_t.
 *
 * @param seg the segment_t to free
 *
 * @return the operation error code
 */
int segment_free(segment_t *seg)
{
	/* Decrease the usage count of the data */
	if (seg->data_usage_func != NULL)
		(*seg->data_usage_func)(seg->data, -1);

	free(seg);

	return 0;
}

/**
 * Clears a segment_t.
 *
 * @param seg the segment_t to clear
 *
 * @return the operation error code
 */
int segment_clear(segment_t *seg)
{
	if (seg == NULL)	
		return_error(EINVAL);

	seg->start = -1;
	seg->size = 0;

	return 0;
}

/**
 * Splits a segment.
 *
 * This functions splits a segment into two. The original segment is changed
 * in-place and a new one is created. The caller is responsible for managing
 * the new segment (eg freeing it).
 *
 * @param seg the segment_t to split
 * @param[out] seg1 the created new segment
 * @param split_index the index in the original segment_t that will be the 
 *                    start of the new segment_t
 *
 * @return the operation error code
 */
int segment_split(segment_t *seg, segment_t **seg1, off_t split_index)
{
	int err = 0;

	if (seg == NULL || seg1 == NULL)
		return_error(EINVAL);

	*seg1 = NULL;

	off_t size = seg->size;
	off_t start = seg->start;
	void *data = seg->data;

	/* is index out of range */
	if (split_index >= size)
		return_error(EINVAL);

	err = segment_new(seg1, data, start + split_index, size - split_index,
			seg->data_usage_func);

	if (err)
		return_error(err);

	/* Change 'seg' second so that if changing 'seg1' fails, 'seg' remains intact */
	if (split_index == 0)
		segment_clear(seg);
	else {
		err = segment_set_range(seg, start, split_index);
		if (err)
			goto fail;
	}

	return 0;

fail:
	segment_free(*seg1);
	*seg1 = NULL;

	return_error(err);
}

/** 
 * Merges two segments.
 *
 * This function merges two related segments. Segment seg1
 * must be a continuation of the first segment for the 
 * merge to succeed. The first segment is changed in place.
 * 
 * @param seg the first segment 
 * @param seg1 the segment to merge with the first segment 
 * 
 * @return 
 */
int segment_merge(segment_t *seg, segment_t *seg1)
{
	if (seg == NULL || seg1 == NULL)
		return_error(EINVAL);

	/* Segments must point to the same object */
	if (seg->data != seg1->data)
		return_error(EINVAL);

	/* Calculate the size of the merged segment */
	if (__MAX(off_t) - seg->size < seg1->size)
		return_error(EOVERFLOW);

	off_t new_size = seg->size + seg1->size;

	/* Make sure the range of the merged segment doesn't overflow */
	if (__MAX(off_t) - seg->start < new_size - 1 * (new_size != 0))
		return_error(EOVERFLOW);

	/* seg1 must be a continuation of seg */
	if (seg->start + seg->size != seg1->start)
		return_error(EINVAL);

	seg->size = new_size;

	return 0;
}

/**
 * Gets data object a segment_t is related to.
 *
 * @param seg the segment_t
 * @param[out] data the data object
 *
 * @return the operation error code
 */
int segment_get_data(segment_t *seg, void  **data)
{
	if (seg == NULL || data == NULL)
		return_error(EINVAL);

	*data = seg->data;

	return 0;
}

/**
 * Gets the start offset of a segment_t.
 *
 * @param seg the segment_t
 * @param[out] start the start offset
 *
 * @return the operation error code
 */
int segment_get_start(segment_t *seg, off_t *start)
{
	if (seg == NULL || start == NULL)
		return_error(EINVAL);

	*start = seg->start;

	return 0;
}

/**
 * Gets the size of a segment_t.
 *
 * @param seg the segment_t
 * @param[out] size the size
 *
 * @return the operation error code
 */
int segment_get_size(segment_t *seg, off_t *size)
{
	if (seg == NULL || size == NULL)
		return_error(EINVAL);

	*size = seg->size;

	return 0;
}

/**
 * Sets the data association of a segment_t.
 *
 * @param seg the segment_t
 * @param data the new data associated with the segment
 * @param data_usage_func the function to call to update the usage count of
 *        the data associated with this segment (may be NULL)
 *
 * @return the operation error code
 */
int segment_set_data(segment_t *seg, void *data,
		segment_data_usage_func data_usage_func)
{
	if (seg == NULL)
		return_error(EINVAL);

	/* Increase the usage count of the new data */
	if (data_usage_func != NULL) {
		int err = (*data_usage_func)(data, 1);
		if (err)
			return_error(err);
	}

	/* Decrease the usage count of the old data */
	if (seg->data_usage_func != NULL) {
		int err = (*seg->data_usage_func)(seg->data, -1);
		if (err) {
			/* Reset the usage count of the new data */
			(*data_usage_func)(data, 0);
			return_error(err);
		}
	}

	seg->data = data;
	seg->data_usage_func = data_usage_func;

	return 0;
}

/**
 * Sets the range of a segment_t.
 *
 * @param seg the segment_t
 * @param start the new start offset
 * @param size the size of the segment
 *
 * @return the operation error code
 */
int segment_set_range(segment_t *seg, off_t start, off_t size)
{
	if (seg == NULL || start < 0 || size < 0)
		return_error(EINVAL);

	/* Check if range would overflow off_t */
	if (__MAX(off_t) - start < size - 1 * (size != 0))
		return_error(EOVERFLOW);

	seg->start = start;
	seg->size = size;

	return 0;
}

