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

segment_t *segment_new()
{
	return NULL;
}

segment_t *segment_split(segment_t *segment, off_t split_index)
{
	return NULL;
}

off_t segment_get_start(segment_t *seg)
{
	return -1;
}

off_t segment_get_end(segment_t *seg)
{
	return -1;
}

size_t segment_get_size(segment_t *seg)
{
	return 0;
}

int segment_change(segment_t *seg, off_t start, off_t end)
{
	return -1;
}

