/**
 * @file segment.c
 *
 * Segment implementation
 */
#include <stdlib.h>
#include <errno.h>
#include "segment.h"

struct segment {
	void *data;
	off_t start;
	size_t size;
	segment_data_usage_func data_usage_func;
};

/**
 * Creates a new empty segment_t.
 *
 * @param[out] seg the created segment or NULL
 * @param data the data object this segment is associated with
 * @param data_usage_func the function to call to update the usage count of
 *        the data associated with this segment (may be NULL)
 *
 * @return the operation error code
 */
int segment_new(segment_t **seg, void *data,
		segment_data_usage_func data_usage_func)
{
	segment_t *segp = NULL;

	segp = (segment_t *) malloc(sizeof(segment_t));

	if (segp == NULL)
		return errno;

	segp->data = data;
	segp->start = -1;
	segp->size = 0;
	segp->data_usage_func = data_usage_func;

	if (data_usage_func != NULL)
		(*data_usage_func)(data, 1);

	*seg = segp;

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
		return EINVAL;

	seg->start = -1;
	seg->size = 0;

	return 0;
}

/**
 * Splits a segment.
 *
 * @param seg the segment_t to split
 * @param[out] seg1 the new segment
 * @param split_index the index of the point in the segment_t to split at
 *
 * @return the operation error code
 */
int segment_split(segment_t *seg, segment_t **seg1, off_t split_index)
{
	int err = 0;

	if (seg == NULL || seg1 == NULL)
		return EINVAL;

	*seg1 = NULL;

	/* Get all info using functions instead of direct structure 
	 * accesss to make this implementation-independent */
	size_t size;
	off_t start;
	void *data;

	segment_get_size(seg, &size);
	segment_get_start(seg, &start);
	segment_get_data(seg, &data);

	/* is index out of range */
	if (split_index >= size)
		return EINVAL;

	err = segment_new(seg1, data, seg->data_usage_func);
	if (err)
		return err;

	err = segment_change(*seg1, start + split_index, size - split_index);
	if (err)
		goto fail;
	
	/* Change 'seg' second so that if changing 'seg1' fails, 'seg' remains intact */
	if (split_index == 0)
		segment_clear(seg);
	else {
		err = segment_change(seg, start, split_index);
		if (err)
			goto fail;
	}

	return 0;

fail:
	segment_free(*seg1);
	*seg1 = NULL;

	return err;
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
	if (seg == NULL)
		return EINVAL;

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
	if (seg == NULL)
		return EINVAL;

	*start = seg->start;

	return 0;
}

/**
 * Gets the end offset of a segment_t.
 *
 * @param seg the segment_t
 * @param[out] end the end offset
 *
 * @return the operation error code
 */
int segment_get_end(segment_t *seg, off_t *end)
{
	if (seg == NULL)
		return EINVAL;

	*end = seg->start + seg->size - 1;

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
int segment_get_size(segment_t *seg, size_t *size)
{
	if (seg == NULL || size == NULL)
		return EINVAL;

	*size = seg->size;

	return 0;
}

/**
 * Changes the range of a segment_t.
 *
 * @param seg the segment_t
 * @param start the new start offset
 * @param end the new end offset
 *
 * @return the operation error code
 */
int segment_change(segment_t *seg, off_t start, size_t size)
{
	if (seg == NULL)
		return EINVAL;

	seg->start = start;
	seg->size = size;

	return 0;
}


