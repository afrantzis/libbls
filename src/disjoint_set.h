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
 * @file disjoint_set.h
 *
 * Disjoint-set API
 */
#ifndef _BLESS_DISJOINT_SET_H
#define _BLESS_DISJOINT_SET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/**
 * @defgroup disjoint_set Disjoint-Set
 *
 * A disjoint-set implementation using a union-find algorithm.
 * @{
 */

/**
 * Opaque type for a disjoint-set.
 */
typedef struct disjoint_set disjoint_set_t;

int disjoint_set_new(disjoint_set_t **ds, size_t size);

int disjoint_set_free(disjoint_set_t *ds);

int disjoint_set_union(disjoint_set_t *ds, size_t id1, size_t id2);

int disjoint_set_find(disjoint_set_t *ds, size_t *set_id, size_t id);

/** @} */

#ifdef __cplusplus
}
#endif

#endif 
