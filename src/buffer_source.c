/**
 * @file buffer_source.c
 *
 * Buffer source implementation
 */
#include "buffer_source.h"
#include "data_object.h"

#include <errno.h>
#include <stdlib.h>

/**
 * Creates a memory source for bless_buffer_t.
 *
 * If the data_free function is NULL the data won't be freed
 * when this source object is freed.
 *
 * @param[out] src the created bless_buffer_source_t.
 * @param data the data related to the source
 * @param length the length of the data
 * @param data_free the function to call to free the data
 *
 * @return the operation error code
 */
int bless_buffer_source_memory(bless_buffer_source_t **src, void *data,
		size_t length, bless_data_free_func *data_free)
{
	if (src == NULL || data == NULL || length < 0)
		return EINVAL;

	/* Create the data object and set its data free function */
	data_object_t *obj;
	int err = data_object_memory_new(&obj, data, length);
	if (err)
		return err;

	err = data_object_set_data_free_func(obj, data_free);
	if (err) {
		data_object_free(obj);
		return err;
	}

	/* 
	 * Increase the usage count of this data object. This is done 
	 * so that the user is able to use it safely multiple times
	 * eg appending various parts of it to the buffer. When the
	 * user is done they should call bless_buffer_source_unref().
	 */
	err = data_object_update_usage(obj, 1);
	if (err) {
		data_object_free(obj);
		return err;
	}

	*src = obj;

	return 0;
}

/**
 * Creates a file source for bless_buffer_t.
 *
 * If the data_free function is NULL the file won't be closed
 * when this source object is freed.
 *
 * @param[out] src the created bless_buffer_source_t.
 * @param fd the file descriptor associated with this source object 
 * @param data_free the function to call to close the file
 *
 * @return the operation error code
 */
int bless_buffer_source_file(bless_buffer_source_t **src, int fd, 
		bless_data_free_func *data_free)
{
	if (src == NULL)
		return EINVAL;

	/* Create the data object and set its data free function */
	data_object_t *obj;
	int err = data_object_file_new(&obj, fd);
	if (err)
		return err;

	err = data_object_set_data_free_func(obj, data_free);
	if (err) {
		data_object_free(obj);
		return err;
	}

	/* 
	 * Increase the usage count of this data object. This is done 
	 * so that the user is able to use it safely multiple times
	 * eg appending various parts of it to the buffer. When the
	 * user is done they should call bless_buffer_source_unref().
	 */
	err = data_object_update_usage(obj, 1);
	if (err) {
		data_object_free(obj);
		return err;
	}

	*src = obj;

	return 0;
}

/**
 * Decreases the usage count of a source object.
 *
 * This function should be called when the user is done
 * using a source object. Failing to do so will lead to
 * a memory leak.
 *
 * @param src the source object to decrease the usage count of
 *
 * @return the operation error code
 */
int bless_buffer_source_unref(bless_buffer_source_t *src)
{
	if (src == NULL)
		return EINVAL;

	data_object_t *obj = (data_object_t *) src;

	int err = data_object_update_usage(obj, -1);
	if (err)
		return err;

	return 0;
}

