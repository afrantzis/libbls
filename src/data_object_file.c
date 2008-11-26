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


/* forward declarations */
static int data_object_file_get_size(data_object_t *obj, off_t *size);
static int data_object_file_free(data_object_t *obj);
static int data_object_file_get_data(data_object_t *obj, void **buf, 
		off_t offset, size_t *length, data_object_flags flags);

/* Function pointers for the file implementation of data_object_t */
static struct data_object_funcs data_object_file_funcs = {
	.get_data = data_object_file_get_data,
	.free = data_object_file_free,
	.get_size = data_object_file_get_size
};

/* Private data for the file implementation of data_object_t */
struct data_object_file_impl {
	int fd;
	off_t size;

	void *page_data;
	off_t page_offset;
	long page_size;	
};

/**
 * Creates a new file data object.
 *
 * If the data object is successfully created it gains ownership
 * of the file descriptor passed to it. When the data object is
 * freed it also closes the associated file descriptor.
 *
 * @param[out] obj the created data object
 * @param fd the file descriptor of the file to use
 *
 * @return the operation error code
 */
int data_object_file_new(data_object_t **obj, int fd)
{
	if (obj == NULL)
		return EINVAL;
		
	if (fd < 0)
		return EBADF;

	/* Allocate memory for implementation */
	struct data_object_file_impl *impl =
		malloc (sizeof(struct data_object_file_impl));

	if (impl == NULL)
		return ENOMEM;

	/* Create data object with file implementation */
	int err = data_object_create_impl(obj, impl, &data_object_file_funcs);

	if (err) {
		free(impl);
		return err;
	}

	impl->fd = fd;

	/* Get size of file */
	impl->size = lseek(fd, 0, SEEK_END);
	if (impl->size == -1) {
		err = errno;
		goto fail;
	}

	impl->page_size = sysconf(_SC_PAGESIZE);
	if (impl->page_size == -1) {
		err = errno;
		goto fail;
	}

	impl->page_data = NULL;

	return 0;

fail:
	free(impl);
	return err;
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
		off_t offset, size_t *length, data_object_flags flags)
{
	if (obj == NULL || buf == NULL || length == NULL || offset < 0)
		return EINVAL;

	size_t len = *length;

	/* Check for overflow */
	if (__MAX(off_t) - offset < len - 1 * (len != 0))
		return EOVERFLOW;

	struct data_object_file_impl *impl =
		data_object_get_impl(obj);

	/* Make sure that the range is valid */
	if (offset + len - 1 * (len != 0) >= impl->size)
		return EINVAL;

	/* If requested data is not loaded in memory, load it... */
	if (impl->page_data == NULL || offset < impl->page_offset
			|| offset >= impl->page_offset + impl->page_size)
	{
		int err;

		/* Unload previous page */
		if (impl->page_data != NULL) {
			err = munmap(impl->page_data, impl->page_size);
			if (err)
				return errno;

			impl->page_data = NULL;
		}

		/* Load new page */
		off_t page_offset = (offset / impl->page_size) * impl->page_size;

		void *mmap_addr = mmap(NULL, impl->page_size, PROT_READ, MAP_PRIVATE,
				impl->fd, page_offset);

		if (mmap_addr == MAP_FAILED)
			return errno;

		impl->page_data = mmap_addr;
		impl->page_offset = page_offset;
	}

	/* Find out if we have loaded more or less than we needed */
	off_t loaded_length = impl->page_size - (offset - impl->page_offset);
	if (loaded_length > len)
		*length = len;
	else
		*length = loaded_length;

	*buf = impl->page_data + offset - impl->page_offset;

	return 0;
}

static int data_object_file_free(data_object_t *obj)
{
	if (obj == NULL)
		return EINVAL;

	struct data_object_file_impl *impl =
		data_object_get_impl(obj);

	close(impl->fd);

	free(impl);

	return 0;
}

static int data_object_file_get_size(data_object_t *obj, off_t *size)
{
	if (obj == NULL || size == NULL)
		return EINVAL;

	struct data_object_file_impl *impl =
		data_object_get_impl(obj);

	*size = impl->size;

	return 0;
}
