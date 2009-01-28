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
 * @file data_object.c
 *
 * Implementation of the base data_object_t ADT and interface mechanism.
 */

#include <stdlib.h>
#include <errno.h>

#include "data_object.h"
#include "data_object_internal.h"
#include "util.h"

struct data_object {
	void *impl;
	struct data_object_funcs *funcs;
	int usage;
};

/**********************
 * Internal functions *
 **********************/

/**
 * Creates a data_object_t using a specific implementation.
 *
 * This functions is for use by the various data_object_t implementations.
 *
 * @param[out] obj the created data_object_t
 * @param impl the implementation private data
 * @param funcs function pointers to the implementations' functions
 *
 * @return the operation status code
 */
int data_object_create_impl(data_object_t **obj, void *impl,
		struct data_object_funcs *funcs)
{
	if (obj == NULL)
		return_error(EINVAL);

	*obj = malloc(sizeof(data_object_t));

	if (*obj == NULL)
		return_error(ENOMEM);

	(*obj)->impl = impl;
	(*obj)->funcs = funcs;
	(*obj)->usage = 0;

	return 0;
	
}

/**
 * Gets the private implementation data of a data_object_t,
 */
void *data_object_get_impl(data_object_t *obj)
{
	return obj->impl;
}

/***************** 
 * API functions *
 *****************/


/** 
 * Gets a pointer to data from the data object.
 *
 * This function provides a pointer that points to a location that contains
 * the data range requested. It is possible that a call to this function
 * retrieves only some of the requested data. Subsequent calls may be needed to
 * complete the whole operation.
 *
 * Furthermore, the pointer returned is valid only as long as no other access
 * is made to the data object (watch out for concurrency issues!).
 *
 * @param obj the obj to get the data from
 * @param[out] buf the location that will contain the data
 * @param offset the offset in the data object to get data from
 * @param[in,out] length the length of the data to get, on return will contain
 *                       the length of the data that was actually retrieved
 * @param flags whether the data will be read or written to (or both)
 *
 * @return the operation error code
 */

int data_object_get_data(data_object_t *obj, void **buf, off_t offset,
		size_t *length, data_object_flags flags)
{
	return (*obj->funcs->get_data)(obj, buf, offset, length, flags);
}

/**
 * Frees the data object and its resources.
 *
 * @param obj the data object to free
 *
 * @return the operation error code
 */
int data_object_free(data_object_t *obj)
{
	int err = (*obj->funcs->free)(obj);

	if (err)
		return_error(err);

	free(obj);

	return 0;
}

/**
 * Updates the usage count of this data object.
 *
 * If the usage count falls to zero (or below) the data object
 * is freed. This function is to be called by the memory management 
 * system of segment_t.
 *
 * @param obj the data object
 * @param change the change in the usage count or 0 to reset the count
 *
 * @return the operation error code
 */
int data_object_update_usage(void *obj, int change)
{
	if (obj == NULL)
		return_error(EINVAL);

	data_object_t *data_obj = (data_object_t *) obj;

	if (change == 0) {
		data_obj->usage = 0;
		return 0;
	}

	data_obj->usage += change;

	if (data_obj->usage <= 0) {
		int err = data_object_free(data_obj);
		if (err)
			return_error(err);
	}

	return 0;
}
/**
 * Gets the size of the data object.
 *
 * @param obj the data object to get the size of
 * @param[out] size the size of the data object
 *
 * @return the operation error code
 */
int data_object_get_size(data_object_t *obj, off_t *size)
{
	return (*obj->funcs->get_size)(obj, size);
}

/** 
 * Compares the data held by two data objects.
 * 
 * Note that the compare is based on reference equality (eg for memory
 * data objects if their data is located at the same memory area, for file
 * objects if they are associated with the same file) not byte by byte
 * comparison. 
 *
 * @param[out] result 0 if they are equal, 1 otherwise
 * @param obj1 one of the data objects to compare
 * @param obj2 the other data object to compare
 * 
 * @return the operation error code 
 */
int data_object_compare(int *result, data_object_t *obj1, data_object_t *obj2)
{
	if (obj1 == NULL || obj2 == NULL || result == NULL)
		return_error(EINVAL);

	/* Check if they are of the same type */
	if (obj1->funcs != obj2->funcs) {
		*result = 1;
		return 0;
	}

	return (*obj1->funcs->compare)(result, obj1, obj2);
}
