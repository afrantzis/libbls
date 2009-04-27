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
 * @file segcol.h
 *
 * Segment Collection ADT
 */
#ifndef _SEGCOL_H
#define _SEGCOL_H

#include <sys/types.h>
#include "segment.h"

/**
 * @defgroup segcol Segment Collection
 *
 * A Segment Collection manages a collection of segments virtually arranged
 * in a continuous linear space.
 *
 * There may be many implementations of the Segment Collection ADT, each with
 * its own performance characteristics. To create a Segment Collection one
 * must use the constructor functions defined in each implementation 
 * (eg segcol_list_new()).
 *
 * A note on error codes: all functions return EINVAL when an invalid range
 * is specified (eg an offset is outside the limits of the virtual space of
 * a Segment Collection).
 *
 * @{
 */

/**
 * Opaque type for segment collection ADT.
 */
typedef struct segcol segcol_t;

/**
 * Opaque type for segment collection iterator.
 */
typedef struct segcol_iter segcol_iter_t;

int segcol_free(segcol_t *segcol);

int segcol_append(segcol_t *segcol, segment_t *seg); 

int segcol_insert(segcol_t *segcol, off_t offset, segment_t *seg); 

int segcol_delete(segcol_t *segcol, segcol_t **deleted, off_t offset, off_t length);

int segcol_find(segcol_t *segcol, segcol_iter_t **iter, off_t offset);

/**
 * @name Iterator functions
 *
 * @{
 */
int segcol_iter_new(segcol_t *segcol, segcol_iter_t **iter);

int segcol_iter_next(segcol_iter_t *iter);

int segcol_iter_is_valid(segcol_iter_t *iter, int *valid);

int segcol_iter_get_segment(segcol_iter_t *iter, segment_t **seg);

int segcol_iter_get_mapping(segcol_iter_t *iter, off_t *mapping);

int segcol_iter_free(segcol_iter_t *iter);

/** @} */

int segcol_get_size(segcol_t *segcol, off_t *size);

/** @} */

#endif /* _SEGCOL_H */
