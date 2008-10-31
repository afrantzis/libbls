/**
 * @file segcol.h
 *
 * Segment Collection ADT
 *
 * @author Alexandros Frantzis
 */
#ifndef _SEGCOL_H
#define _SEGCOL_H

#include <sys/types.h>
#include "segment.h"

/**
 * @defgroup segcol Segment Collection ADT.
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
