/**
 * @file segment.h
 *
 * Segment
 */
#ifndef _SEGMENT_H
#define _SEGMENT_H

#include <sys/types.h>

/**
 * Opaque type for segment ADT.
 */
typedef struct segment segment_t;

/**
 * Function called to change the usage count of some data.
 *
 * @param data the data whose usage count is being changed
 * @param change the change in the usage count
 */
typedef void (*segment_data_usage_func)(void *data, int change);

int segment_new(segment_t **seg, void *data, off_t start, size_t size,
		segment_data_usage_func data_usage_func);

int segment_copy(segment_t *seg, segment_t **seg_copy);

int segment_free(segment_t *seg);

int segment_clear(segment_t *seg);

int segment_split(segment_t *seg, segment_t **seg1, off_t split_index);

int segment_get_data(segment_t *seg, void **data);

int segment_get_start(segment_t *seg, off_t *start);

int segment_get_end(segment_t *seg, off_t *end);

int segment_get_size(segment_t *seg, size_t *size);

int segment_change_data(segment_t *seg, void *data,
		segment_data_usage_func data_usage_func);

int segment_change_range(segment_t *seg, off_t start, size_t size);

#endif /* _SEGMENT_H */
