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
 * @file data_object_file.c
 *
 * Implementation of the file data_object_t.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "data_object.h"
#include "data_object_internal.h"
#include "data_object_file.h"
#include "type_limits.h"
#include "debug.h"
#include "util.h"


/* forward declarations */
static int data_object_file_get_size(data_object_t *obj, off_t *size);
static int data_object_file_free(data_object_t *obj);
static int data_object_file_get_data(data_object_t *obj, void **buf, 
		off_t offset, off_t *length, data_object_flags flags);
static int data_object_file_compare(int *result, data_object_t *obj1,
		data_object_t *obj2);

/* Function pointers for the file implementation of data_object_t */
static struct data_object_funcs data_object_file_funcs = {
	.get_data = data_object_file_get_data,
	.free = data_object_file_free,
	.get_size = data_object_file_get_size,
	.compare = data_object_file_compare
};

/* Private data for the file implementation of data_object_t */
struct data_object_file_impl {
	int fd;
	off_t size;

	void *page_data;
	off_t page_offset;
	long page_size;	

	/* Device and inode of fd. Used in data object comparisons. */
	dev_t dev;
	ino_t inode;

	data_object_file_close_func *file_close;

	/* The path of the file (only used in tempfile data objects */
	char *path;
};

/**
 * Creates a new file data object.
 *
 * The data object by default doesn't own the file passed to it.
 * That means that when the data object is freed the file will
 * not be closed. To change data ownership by the data_object_t
 * use data_object_set_data_ownership().
 *
 * @param[out] obj the created data object
 * @param fd the file descriptor of the file to use
 *
 * @return the operation error code
 */
int data_object_file_new(data_object_t **obj, int fd)
{
	if (obj == NULL)
		return_error(EINVAL);
		
	if (fd < 0)
		return_error(EBADF);

	/* Allocate memory for implementation */
	struct data_object_file_impl *impl =
		malloc (sizeof(struct data_object_file_impl));

	if (impl == NULL)
		return_error(ENOMEM);

	/* Create data object with file implementation */
	int err = data_object_create_impl(obj, impl, &data_object_file_funcs);
	if (err)
		goto_error(err, on_error_object);

	impl->fd = fd;

	/* Get file info */
	struct stat st;
	err = fstat(fd, &st);
	if (err)
		goto_error(errno, on_error_other);

	impl->dev = st.st_dev;
	impl->inode = st.st_ino;

	/* Get size of file */
	impl->size = lseek(fd, 0, SEEK_END);
	if (impl->size == -1)
		goto_error(errno, on_error_other);

	impl->page_size = sysconf(_SC_PAGESIZE);
	if (impl->page_size == -1)
		goto_error(errno, on_error_other);

	impl->page_data = NULL;

	/* We don't own the file by default */
	impl->file_close = NULL;
	
	impl->path = NULL;

	return 0;

on_error_other:
	free(obj);
on_error_object:
	free(impl);
	return err;
}

/**
 * Creates a new temporary file data object.
 *
 * When the data object is freed the temporary file will be deleted
 * from the file system.
 *
 * The data object by default doesn't own the file passed to it.
 * That means that when the data object is freed the file will
 * not be closed. To change data ownership by the data_object_t
 * use data_object_set_data_ownership().
 *
 * @param[out] obj the created data object
 * @param fd the file descriptor of the file to use
 * @param path the path of the file the file descriptor points to 
 *
 * @return the operation error code
 */
int data_object_tempfile_new(data_object_t **obj, int fd, char *path)
{
	int err = data_object_file_new(obj, fd);
	if (err)
		return_error(err);

	struct data_object_file_impl *impl =
		data_object_get_impl(*obj);

	/* Store path */
	size_t path_len = strlen(path);
	
	impl->path = malloc(path_len + 1);
	if (impl->path == NULL) {
		data_object_free(*obj);
		return_error(ENOMEM);
	}

	strncpy(impl->path, path, path_len + 1);

	return 0;
}
/**
 * Sets the function used to close the file associated with the data object.
 *
 * If the function is NULL, the file wont be closed when the data object is
 * freed.
 *
 * @param obj the data object 
 * @param file_close the function used to close the file associated with the 
 *                   data object
 *
 * @return the operation error code
 */
int data_object_file_set_close_func(data_object_t *obj,
        data_object_file_close_func *file_close)
{
	if (obj == NULL)
		return_error(EINVAL);

	struct data_object_file_impl *impl =
		data_object_get_impl(obj);

	impl->file_close = file_close;

	return 0;
}

/*
 * The file data object provides access to a file's data by partially mapping a
 * page of it in memory. When a caller requests a data range we check if a part
 * of the range is already loaded in memory. If it is, we return a pointer to
 * that memory area. If the whole range was not returned it is up to the caller
 * to call the function again (perhaps multiple times) to retrieve the data.
 *
 * If the data range requested is not in memory we first unmap the previous
 * mapped file page (if any). We then map a new page that contains a part from
 * the start of requested range (perhaps all of it).
 */
static int data_object_file_get_data(data_object_t *obj, void **buf, 
		off_t offset, off_t *length, data_object_flags flags)
{
	UNUSED_PARAM(flags);

	if (obj == NULL || buf == NULL || length == NULL || offset < 0)
		return_error(EINVAL);

	off_t len = *length;

	/* Check for overflow */
	if (__MAX(off_t) - offset < len - 1 * (len != 0))
		return_error(EOVERFLOW);

	struct data_object_file_impl *impl =
		data_object_get_impl(obj);

	/* Make sure that the range is valid */
	if (offset + len - 1 * (len != 0) >= impl->size)
		return_error(EINVAL);

	/* If requested data is not loaded in memory, load it... */
	if (impl->page_data == NULL || offset < impl->page_offset
			|| offset >= impl->page_offset + impl->page_size)
	{
		int err;

		/* Unload previous page */
		if (impl->page_data != NULL) {
			err = munmap(impl->page_data, impl->page_size);
			if (err)
				return_error(errno);

			impl->page_data = NULL;
		}

		/* Load new page */
		off_t page_offset = (offset / impl->page_size) * impl->page_size;

		void *mmap_addr = mmap(NULL, impl->page_size, PROT_READ, MAP_PRIVATE,
				impl->fd, page_offset);

		if (mmap_addr == MAP_FAILED)
			return_error(errno);

		impl->page_data = mmap_addr;
		impl->page_offset = page_offset;
	}

	/* Find out if we have loaded more or less than we needed */
	off_t loaded_length = impl->page_size - (offset - impl->page_offset);
	if (loaded_length > len)
		*length = len;
	else
		*length = loaded_length;

	*buf = (unsigned char *)impl->page_data + offset - impl->page_offset;

	return 0;
}

static int data_object_file_free(data_object_t *obj)
{
	if (obj == NULL)
		return_error(EINVAL);

	struct data_object_file_impl *impl =
		data_object_get_impl(obj);

	/* If we have mapped data, unmap them */
	if (impl->page_data != NULL) {
		int err = munmap(impl->page_data, impl->page_size);
		if (err == -1)
			return_error(errno);
	}

	/* Free the data (close the file) */
    data_object_file_close_func *file_close = impl->file_close;

	if (file_close != NULL) {
		int err = file_close(impl->fd);
		if (err)
			return_error(err);
	}

	/* If we have a path (this is a temp file) remove the path */
	if (impl->path != NULL) {
		unlink(impl->path);
		free(impl->path);
	}

	free(impl);

	return 0;
}

static int data_object_file_get_size(data_object_t *obj, off_t *size)
{
	if (obj == NULL || size == NULL)
		return_error(EINVAL);

	struct data_object_file_impl *impl =
		data_object_get_impl(obj);

	*size = impl->size;

	return 0;
}

static int data_object_file_compare(int *result, data_object_t *obj1,
		data_object_t *obj2)
{
	if (obj1 == NULL || obj2 == NULL || result == NULL)
		return_error(EINVAL);

	struct data_object_file_impl *impl1 =
		data_object_get_impl(obj1);

	struct data_object_file_impl *impl2 =
		data_object_get_impl(obj2);

	*result = !((impl1->dev == impl2->dev) && (impl1->inode == impl2->inode));

	return 0;
}

