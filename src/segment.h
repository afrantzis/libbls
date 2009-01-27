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
 * Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file segment.h
 *
 * Segment
 */
#ifndef _SEGMENT_H
#define _SEGMENT_H

#include <sys/types.h>

/**
 * @defgroup segment Segment
 *
 * A segment holds information about a continuous part of a data object.
 *
 * By default, when a segment becomes associated with a data object, the
 * segment is not responsible for managing the data object's memory. This
 * kind of memory management can be achieved be specifying the data_usage_func
 * when creating the segment or changing its data association. The function 
 * is called whenever the segment is created or freed and updates the data 
 * object's usage count.
 *
 * @{
 */

/**
 * Opaque type for segment ADT.
 */
typedef struct segment segment_t;

/**
 * Function called to change the usage count of some data.
 *
 * @param data the data whose usage count is being changed
 * @param change the change in the usage count or 0 to reset the count
 *
 * @return the operation error code
 */
typedef int (*segment_data_usage_func)(void *data, int change);

int segment_new(segment_t **seg, void *data, off_t start, off_t size,
		segment_data_usage_func data_usage_func);

int segment_copy(segment_t *seg, segment_t **seg_copy);

int segment_free(segment_t *seg);

int segment_clear(segment_t *seg);

int segment_split(segment_t *seg, segment_t **seg1, off_t split_index);

int segment_merge(segment_t *seg, segment_t *seg1);

int segment_get_data(segment_t *seg, void **data);

int segment_get_start(segment_t *seg, off_t *start);

int segment_get_size(segment_t *seg, off_t *size);

int segment_set_data(segment_t *seg, void *data,
		segment_data_usage_func data_usage_func);

int segment_set_range(segment_t *seg, off_t start, off_t size);

/** @} */

#endif /* _SEGMENT_H */
