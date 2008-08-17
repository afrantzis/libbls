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
 * @return the new segment_t or NULL on error
 */
segment_t *segment_new()
{
	segment_t *seg;

	seg = (segment_t *) malloc(sizeof(segment_t));
	seg->start = -1;
	seg->end = -1;

	return seg;
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

segment_t *segment_split(segment_t *segment, off_t split_index)
{
	return NULL;
}

/**
 * Gets the start offset of a segment_t.
 *
 * @param seg the segment_t
 *
 * @return the starting offset or -1 on error
 */
off_t segment_get_start(segment_t *seg)
{
	if (seg == NULL)
		return -1;

	return seg->start;
}

/**
 * Gets the end offset of a segment_t.
 *
 * @param seg the segment_t
 *
 * @return the end offset or -1 on error
 */
off_t segment_get_end(segment_t *seg)
{
	if (seg == NULL)
		return -1;

	return seg->end;
}

ssize_t segment_get_size(segment_t *seg)
{
	if (seg == NULL)
		return -1;

	if (seg->start == -1 && seg->end == -1)
		return 0;

	return seg->end - seg->start + 1;
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


