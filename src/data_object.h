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
 * @file data_object.h
 *
 * Data object ADT API.
 */
#ifndef _DATA_OBJECT_H
#define _DATA_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdint.h>

/**
 * @defgroup data_object Data Object
 *
 * A data object is an abstraction for objects that can provide data.
 * It provides a common API to transparently get the data 
 * regardless of the particular source they come from.
 *
 * The two main data object types are the memory data object and the file
 * data object. To create a data object one must use the provided constructor
 * for the particular type.
 *
 * @{
 */

/**
 * Opaque type for data object ADT.
 */
typedef struct data_object data_object_t;


/** 
 * Flags for the usage of data returned by data_object_get_data().
 */
typedef enum { 
	DATA_OBJECT_READ = 1, /**< Data will be used just for reading */
	DATA_OBJECT_WRITE = 2, /**< Data will be used just for writing */
	DATA_OBJECT_RW = 3 /**< Data will be used for both reading and writing */
} data_object_flags;

int data_object_get_data(data_object_t *obj, void **buf, off_t offset,
		off_t *length, data_object_flags flags);

int data_object_free(data_object_t *obj);

int data_object_update_usage(void *obj, int change);

int data_object_get_size(data_object_t *obj, off_t *size);

int data_object_compare(int *result, data_object_t *obj1, data_object_t *obj2);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
