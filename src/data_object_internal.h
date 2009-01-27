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
 * @file data_object_internal.h
 *
 * Definitions for internal use by the data_object_t implementations.
 */
#ifndef _DATA_OBJECT_INTERNAL_H
#define _DATA_OBJECT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "data_object.h"

/**
 * The functions in the data_object ADT that must be implemented.
 */
struct data_object_funcs {
	int (*get_data)(data_object_t *obj, void **buf, off_t offset,
		size_t *length, data_object_flags flags);
	int (*free)(data_object_t *obj);
	int (*get_size)(data_object_t *obj, off_t *size);
	int (*compare)(int *result, data_object_t *obj1, data_object_t *obj2);
};

int data_object_create_impl(data_object_t **obj, void *impl,
		struct data_object_funcs *funcs);

void *data_object_get_impl(data_object_t *obj);

#ifdef __cplusplus
}
#endif

#endif

