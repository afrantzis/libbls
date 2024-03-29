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
 * @file buffer_source.c
 *
 * Buffer source implementation
 */

#include "buffer_source.h"
#include "data_object.h"
#include "data_object_memory.h"
#include "data_object_file.h"
#include "debug.h"


#include <errno.h>
#include <stdlib.h>

#pragma GCC visibility push(default)

/**
 * Creates a memory source for bless_buffer_t.
 *
 * If the mem_free function is NULL the data won't be freed
 * when this source object is freed.
 *
 * @param[out] src the created bless_buffer_source_t.
 * @param data the data related to the source
 * @param length the length of the data
 * @param mem_free the function to call to free the data
 *
 * @return the operation error code
 */
int bless_buffer_source_memory(bless_buffer_source_t **src, void *data,
		size_t length, bless_mem_free_func *mem_free)
{
	if (src == NULL || data == NULL)
		return_error(EINVAL);

	/* Create the data object and set its data free function */
	data_object_t *obj;
	int err = data_object_memory_new(&obj, data, length);
	if (err)
		return_error(err);

	err = data_object_memory_set_free_func(obj, mem_free);
	if (err)
		goto_error(err, on_error);

	/* 
	 * Increase the usage count of this data object. This is done 
	 * so that the user is able to use it safely multiple times
	 * eg appending various parts of it to the buffer. When the
	 * user is done they should call bless_buffer_source_unref().
	 */
	err = data_object_update_usage(obj, 1);
	if (err)
		goto_error(err, on_error);

	*src = obj;

	return 0;

on_error:
	data_object_free(obj);
	return err;
}

/**
 * Creates a file source for bless_buffer_t.
 *
 * If the data_free function is NULL the file won't be closed
 * when this source object is freed.
 *
 * @param[out] src the created bless_buffer_source_t.
 * @param fd the file descriptor associated with this source object 
 * @param file_close the function to call to close the file
 *
 * @return the operation error code
 */
int bless_buffer_source_file(bless_buffer_source_t **src, int fd, 
		bless_file_close_func *file_close)
{
	if (src == NULL)
		return_error(EINVAL);

	/* Create the data object and set its data free function */
	data_object_t *obj;
	int err = data_object_file_new(&obj, fd);
	if (err)
		return_error(err);

	err = data_object_file_set_close_func(obj, file_close);
	if (err)
		goto_error(err, on_error);

	/* 
	 * Increase the usage count of this data object. This is done 
	 * so that the user is able to use it safely multiple times
	 * eg appending various parts of it to the buffer. When the
	 * user is done they should call bless_buffer_source_unref().
	 */
	err = data_object_update_usage(obj, 1);
	if (err)
		goto_error(err, on_error);

	*src = obj;

	return 0;

on_error:
	data_object_free(obj);
	return err;
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
		return_error(EINVAL);

	data_object_t *obj = (data_object_t *) src;

	int err = data_object_update_usage(obj, -1);
	if (err)
		return_error(err);

	return 0;
}

#pragma GCC visibility pop

