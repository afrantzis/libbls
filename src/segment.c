/**
 * @file segment.c
 *
 * Segment implementation
 */
#include <stdlib.h>
#include "segment.h"

struct segment {
	off_t start;
	off_t end;
};

/**
 * Creates a new segment_t.
 *
 * @param[out] seg the created segment or NULL
 *
 * @return the operation status code
 */
int segment_new(segment_t **seg)
{
	segment_t *segp = NULL;

	segp = (segment_t *) malloc(sizeof(segment_t));
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
 * @return the operation status code
 */
int segment_free(segment_t *seg)
{
	free(seg);

	return 0;
}

/**
 * Splits a segment.
 *
 * @param seg the segment_t to split
 * @param[out] seg1 the new segment
 * @param split_index the index in the segment_t to split at
 *
 * @return the operation status code
 */
int segment_split(segment_t *seg, segment_t **seg1, off_t split_index)
{
	return -1;
}

/**
 * Gets the start offset of a segment_t.
 *
 * @param seg the segment_t
 * @param[out] start the start offset
 *
 * @return the operation status code
 */
int segment_get_start(segment_t *seg, off_t *start)
{
	if (seg == NULL)
		return -1;

	*start = seg->start;

	return 0;
}

/**
 * Gets the end offset of a segment_t.
 *
 * @param seg the segment_t
 *
 * @return the end offset or -1 on error
 */
int segment_get_end(segment_t *seg, off_t *end)
{
	if (seg == NULL)
		return -1;

	*end = seg->end;

	return 0;
}

int segment_get_size(segment_t *seg, size_t *size)
{
	if (seg == NULL)
		return -1;

	if (seg->start == -1 && seg->end == -1)
		*size = 0;
	else
		*size = seg->end - seg->start + 1;

	return 0;
}

/**
 * Changes the range of a segment_t.
 *
 * @param start the new start offset
 * @param end the new end offset
 *
 * @return the operation status code
 */
int segment_change(segment_t *seg, off_t start, off_t end)
{
	if (seg == NULL)
		return -1;

	seg->start = start;
	seg->end = end;

	return 0;
}


