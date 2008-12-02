/**
 * @file data_object.c
 *
 * Implementation of the base data_object_t ADT and interface mechanism.
 */
#include <stdlib.h>
#include <errno.h>

#include "data_object.h"
#include "data_object_internal.h"

struct data_object {
	void *impl;
	struct data_object_funcs *funcs;
	int usage;
	data_free_func data_free;
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
		return EINVAL;

	*obj = malloc(sizeof(data_object_t));

	if (*obj == NULL)
		return ENOMEM;

	(*obj)->impl = impl;
	(*obj)->funcs = funcs;
	(*obj)->usage = 0;
	/* We don't own the data by default */
	(*obj)->data_free = NULL;

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
		return err;

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
		return EINVAL;

	data_object_t *data_obj = (data_object_t *) obj;

	if (change == 0) {
		data_obj->usage = 0;
		return 0;
	}

	data_obj->usage += change;

	if (data_obj->usage <= 0) {
		int err = data_object_free(data_obj);
		if (err)
			return err;
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
 * Sets the function used to free the data held by the data object.
 *
 * If the function is NULL, the data won't be freed when the data object is
 * freed.
 *
 * @param obj the data object 
 * @param data_free the function used to free the data held by the data object
 *
 * @return the operation error code
 */
int data_object_set_data_free_func(data_object_t *obj, data_free_func data_free)
{
	if (obj == NULL)
		return EINVAL;

	obj->data_free = data_free;

	return 0;
}

/**
 * Gets the function used to free the data held by the data object.
 *
 * @param obj the data object 
 * @param[out] data_free the function used to free the data held by the data object
 *
 * @return the operation error code
 */
int data_object_get_data_free_func(data_object_t *obj, data_free_func *data_free)
{
	if (obj == NULL)
		return EINVAL;

	*data_free = obj->data_free;

	return 0;
}

