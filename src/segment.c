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
	off_t end;
};

/**
 * Creates a new empty segment_t.
 *
 * @param[out] seg the created segment or NULL
 * @param data the data object this segment is related to
 *
 * @return the operation error code
 */
int segment_new(segment_t **seg, void *data)
{
	segment_t *segp = NULL;

	segp = (segment_t *) malloc(sizeof(segment_t));

	if (segp == NULL)
		return errno;

	segp->data = data;
	segp->start = -1;
	segp->end = -1;

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
	seg->end = -1;

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
	off_t end;
	void *data;

	segment_get_size(seg, &size);
	segment_get_start(seg, &start);
	segment_get_end(seg, &end);
	segment_get_data(seg, &data);

	/* is index out of range */
	if (split_index >= size)
		return EINVAL;

	err = segment_new(seg1, data);
	if (err)
		return err;

	err = segment_change(*seg1, start + split_index, end);
	if (err)
		goto fail;
	
	/* Change 'seg' second so that if changing 'seg1' fails, 'seg' remains intact */
	if (split_index == 0)
		segment_clear(seg);
	else {
		err = segment_change(seg, start, start + split_index - 1);
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

	*end = seg->end;

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

	if (seg->start == -1 && seg->end == -1)
		*size = 0;
	else
		*size = seg->end - seg->start + 1;

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
int segment_change(segment_t *seg, off_t start, off_t end)
{
	if (seg == NULL)
		return EINVAL;

	if (start > end)
		return EINVAL;

	seg->start = start;
	seg->end = end;

	return 0;
}


