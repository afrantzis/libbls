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
 * @name Internal functions
 * 
 * @{
 */
void segcol_register_impl(segcol_t *segcol,
		void *impl,
		int (*free)(segcol_t *segcol),
		int (*insert)(segcol_t *segcol, off_t offset, segment_t *seg),
		segcol_t *(*delete)(segcol_t *segcol, off_t offset, size_t length),
		segcol_iter_t *(*find)(segcol_t *segcol, off_t offset),
		void *(*iter_new)(segcol_t *segcol),
		int (*iter_next)(segcol_iter_t *iter),
		segment_t *(*iter_get_segment)(segcol_iter_t *iter),
		off_t (*iter_get_mapping)(segcol_iter_t *iter),
		int (*iter_is_valid)(segcol_iter_t *iter),
		int (*iter_free)(segcol_iter_t *segcol)
		);

void *segcol_get_impl(segcol_t *segcol);

void *segcol_iter_get_impl(segcol_iter_t *iter);

/** @} */

/** @} */

#endif /* _SEGCOL_INTERNAL_H */
