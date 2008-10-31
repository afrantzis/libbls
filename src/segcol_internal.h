/**
 * @file segcol_internal.h
 *
 * Segcol definitions for internal use
 */
#ifndef _SEGCOL_INTERNAL_H
#define _SEGCOL_INTERNAL_H

#include "segcol.h"

/**
 * @addtogroup segcol
 *
 * @{
 */

/**
 * Struct that holds the function pointers for an implementation of segcol_t.
 */
struct segcol_funcs {
		int (*free)(segcol_t *segcol);
		int (*append)(segcol_t *segcol, segment_t *seg); 
		int (*insert)(segcol_t *segcol, off_t offset, segment_t *seg); 
		int (*delete)(segcol_t *segcol, segcol_t **deleted, off_t offset,
				off_t length);
		int (*find)(segcol_t *segcol, segcol_iter_t **iter, off_t offset);
		int (*iter_new)(segcol_t *segcol, void **iter);
		int (*iter_next)(segcol_iter_t *iter);
		int (*iter_is_valid)(segcol_iter_t *iter, int *valid);
		int (*iter_get_segment)(segcol_iter_t *iter, segment_t **seg);
		int (*iter_get_mapping)(segcol_iter_t *iter, off_t *mapping);
		int (*iter_free)(segcol_iter_t *iter);
};

/**
 * @name Internal functions
 * 
 * @{
 */
int segcol_create_impl(segcol_t **segcol, void *impl,
		struct segcol_funcs *funcs);

void *segcol_get_impl(segcol_t *segcol);

void *segcol_iter_get_impl(segcol_iter_t *iter);

/** @} */

/** @} */

#endif /* _SEGCOL_INTERNAL_H */
