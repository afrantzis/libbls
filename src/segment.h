/**
 * @file segment.h
 *
 * Segment
 */
#ifndef _SEGMENT_H
#define _SEGMENT_H

#include <sys/types.h>

typedef struct segment segment_t;

int segment_new(segment_t **seg, void *data);

int segment_free(segment_t *seg);

int segment_clear(segment_t *seg);

int segment_split(segment_t *seg, segment_t **seg1, off_t split_index);

int segment_get_data(segment_t *seg, void **data);

int segment_get_start(segment_t *seg, off_t *start);

int segment_get_end(segment_t *seg, off_t *end);

int segment_get_size(segment_t *seg, size_t *size);

int segment_change(segment_t *seg, off_t start, size_t size);

#endif /* _SEGMENT_H */
