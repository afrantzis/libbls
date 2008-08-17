/**
 * @file segment.h
 *
 * Segment
 */
#ifndef _SEGMENT_H
#define _SEGMENT_H

#include <sys/types.h>

typedef struct segment segment_t;

segment_t *segment_new();

int segment_free(segment_t *seg);

segment_t *segment_split(segment_t *segment, off_t split_index);

off_t segment_get_start(segment_t *seg);

off_t segment_get_end(segment_t *seg);

ssize_t segment_get_size(segment_t *seg);

int segment_change(segment_t *seg, off_t start, off_t end);

#endif /* _SEGMENT_H */
