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


/* forward declerations */
static int data_object_memory_get_size(data_object_t *obj, size_t *size);
static int data_object_memory_free(data_object_t *obj);
static int data_object_memory_write(data_object_t *obj, off_t offset, void *data,
		size_t len);
static int data_object_memory_read(data_object_t *obj, void **buf, off_t offset,
		size_t len);

/* Function pointers for the memory implementation of data_object_t */
static struct data_object_funcs data_object_memory_funcs = {
	.read = data_object_memory_read,
	.write = data_object_memory_write,
	.free = data_object_memory_free,
	.get_size = data_object_memory_get_size
};

/* Private data for the memory implementation of data_object_t */
struct data_object_memory_impl {
	void *data;
	size_t size;
};

/**
 * Creates a new empty memory data object.
 *
 * @param[out] obj the created data object
 * @param size the size of the data object
 *
 * @return the operation error code
 */
int data_object_memory_new(data_object_t **obj, size_t size)
{
	void *data = malloc(size);

	if (data == NULL)
		return ENOMEM;

	int err = data_object_memory_new_data(obj, data, size);

	if (err) {
		free(data);
		return err;
	}

	return 0;
}

/**
 * Creates a new memory data object initialized with data.
 *
 * On success the data object will own the data (it is responsible for
 * further memory management).
 *
 * @param[out] obj the created data object
 * @param data the data that this data object will contain
 * @param size the size of the data (and the data object)
 *
 * @return the operation error code
 */
int data_object_memory_new_data(data_object_t **obj, void *data, size_t size)
{
	if (obj == NULL)
		return EINVAL;

	/* Allocate memory for implementation */
	struct data_object_memory_impl *impl =
		malloc (sizeof(struct data_object_memory_impl));

	if (impl == NULL)
		return ENOMEM;

	impl->data = data;

	/* Create data_object_t */
	int err = data_object_create_impl(obj, impl, &data_object_memory_funcs);

	if (err) {
		free(impl);
		return err;
	}

	impl->size = size;

	return 0;

}


static int data_object_memory_read(data_object_t *obj, void **buf, off_t offset,
		size_t len)
{
	if (obj == NULL || buf == NULL)
		return EINVAL;

	struct data_object_memory_impl *impl =
		data_object_get_impl(obj);

	/* Make sure that the range is valid */
	/* TODO: Check for overflow */
	if (offset < 0 || offset + len - 1 * (len != 0) >= impl->size)
		return EINVAL;

	*buf = impl->data + offset;

	return 0;
}

static int data_object_memory_write(data_object_t *obj, off_t offset, void *data,
		size_t len)
{
	if (obj == NULL || data == NULL)
		return EINVAL;

	struct data_object_memory_impl *impl =
		data_object_get_impl(obj);

	/* Make sure that the range is valid */
	/* TODO: Check for overflow */
	if (offset < 0 || offset + len - 1 * (len != 0) >= impl->size)
		return EINVAL;

	memcpy(impl->data + offset, data, len);

	return 0;
}

static int data_object_memory_free(data_object_t *obj)
{
	if (obj == NULL)
		return EINVAL;

	struct data_object_memory_impl *impl =
		data_object_get_impl(obj);

	free(impl->data);
	free(impl);

	return 0;
}

static int data_object_memory_get_size(data_object_t *obj, size_t *size)
{
	if (obj == NULL)
		return EINVAL;

	struct data_object_memory_impl *impl =
		data_object_get_impl(obj);

	*size = impl->size;

	return 0;
}