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
 * @file data_object_memory.c
 *
 * Implementation of the memory data_object_t.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "data_object.h"
#include "data_object_internal.h"
#include "data_object_memory.h"
#include "type_limits.h"
#include "util.h"


/* forward declerations */
static int data_object_memory_get_size(data_object_t *obj, off_t *size);
static int data_object_memory_free(data_object_t *obj);
static int data_object_memory_get_data(data_object_t *obj, void **buf, off_t offset,
		off_t *length, data_object_flags flags);
static int data_object_memory_compare(int *result, data_object_t *obj1,
		data_object_t *obj2);

/* Function pointers for the memory implementation of data_object_t */
static struct data_object_funcs data_object_memory_funcs = {
	.get_data = data_object_memory_get_data,
	.free = data_object_memory_free,
	.get_size = data_object_memory_get_size,
	.compare = data_object_memory_compare
};

/* Private data for the memory implementation of data_object_t */
struct data_object_memory_impl {
	void *data;
	size_t size;
	data_object_memory_free_func *mem_free;
};

/**
 * Creates a new memory data object initialized with data.
 *
 * The data object by default doesn't own the data passed to it.
 * That means that when the data object is freed the data will
 * not be freed. To change data ownership by the data_object_t
 * use data_object_set_data_ownership().
 *
 * @param[out] obj the created data object
 * @param data the data that this data object will contain
 * @param size the size of the data (and the data object)
 *
 * @return the operation error code
 */
int data_object_memory_new(data_object_t **obj, void *data, size_t size)
{
	if (obj == NULL)
		return_error(EINVAL);

	/* Check for overflow */
	if (__MAX(size_t) - (size_t)data < size - 1 * (size != 0))
		return_error(EOVERFLOW);

	/* Allocate memory for implementation */
	struct data_object_memory_impl *impl =
		malloc (sizeof(struct data_object_memory_impl));

	if (impl == NULL)
		return_error(ENOMEM);

	impl->data = data;

	/* Create data_object_t */
	int err = data_object_create_impl(obj, impl, &data_object_memory_funcs);

	if (err) {
		free(impl);
		return_error(err);
	}

	impl->size = size;

	/* We don't own the data by default */
    impl->mem_free = NULL;

	return 0;

}

/**
 * Sets the function used to free the data held by the data object.
 *
 * If the function is NULL, the data won't be freed when the data object is
 * freed.
 *
 * @param obj the data object 
 * @param mem_free the function used to free the data held by the data object
 *
 * @return the operation error code
 */
int data_object_memory_set_free_func(data_object_t *obj,
        data_object_memory_free_func *mem_free)
{
	if (obj == NULL)
		return_error(EINVAL);

	struct data_object_memory_impl *impl =
		data_object_get_impl(obj);

	impl->mem_free = mem_free;

	return 0;
}

static int data_object_memory_get_data(data_object_t *obj, void **buf, 
		off_t offset, off_t *length, data_object_flags flags)
{
	if (obj == NULL || buf == NULL || length == NULL || offset < 0)
		return_error(EINVAL);

	off_t len = *length;

	/* Check for overflow */
	if (__MAX(off_t) - offset < len - 1 * (len != 0))
		return_error(EOVERFLOW);

	struct data_object_memory_impl *impl =
		data_object_get_impl(obj);

	/* Make sure that the range is valid */
	if (offset + len - 1 * (len != 0) >= impl->size)
		return_error(EINVAL);

	*buf = (unsigned char *)impl->data + offset;

	/* Return data in chunks whose size fits in ssize_t */
	if (len > __MAX(ssize_t))
		*length = __MAX(ssize_t);

	return 0;
}

static int data_object_memory_free(data_object_t *obj)
{
	if (obj == NULL)
		return_error(EINVAL);

	struct data_object_memory_impl *impl =
		data_object_get_impl(obj);

	/* Free the data */
    data_object_memory_free_func *mem_free = impl->mem_free;

	if (mem_free != NULL)
		mem_free(impl->data);

	free(impl);

	return 0;
}

static int data_object_memory_get_size(data_object_t *obj, off_t *size)
{
	if (obj == NULL)
		return_error(EINVAL);

	struct data_object_memory_impl *impl =
		data_object_get_impl(obj);

	*size = (off_t) impl->size;

	return 0;
}

static int data_object_memory_compare(int *result, data_object_t *obj1,
		data_object_t *obj2)
{
	if (obj1 == NULL || obj2 == NULL || result == NULL)
		return_error(EINVAL);

	struct data_object_memory_impl *impl1 =
		data_object_get_impl(obj1);

	struct data_object_memory_impl *impl2 =
		data_object_get_impl(obj2);

	*result = !((impl1->data == impl2->data) && (impl1->size == impl2->size));

	return 0;
}
