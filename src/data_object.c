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
 * Reads data from the data object.
 *
 * This function will provide a pointer that points to a location to read
 * the data from. This may be the original data (not a copy) and must not be 
 * altered!
 *
 * Furthermore, this pointer is valid only as long as no other access is made
 * to the data object (watch out for concurrency issues!).
 *
 * @param obj the obj to read from
 * @param[out] buf the buf that will point to the location to read data from
 * @param offset the offset in the data object to read from
 * @param len the length of the data to read
 *
 * @return the operation error code
 */
int data_object_read(data_object_t *obj, void **buf, off_t offset, size_t len)
{
	return (*obj->funcs->read)(obj, buf, offset, len);
}

/**
 * Writes data to the data object.
 *
 * @param obj the obj to write to
 * @param offset the offset in the data object to write to
 * @param data the data to write
 * @param len the length of the data to write
 *
 * @return the operation error code
 */
int data_object_write(data_object_t *obj, off_t offset, void *data,
		size_t len)
{
	return (*obj->funcs->write)(obj, offset, data, len);
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
 * Gets the size of the data object.
 *
 * @param obj the data object to get the size of
 * @param[out] size the size of the data object
 *
 * @return the operation error code
 */
int data_object_get_size(data_object_t *obj, size_t *size)
{
	return (*obj->funcs->get_size)(obj, size);
}

