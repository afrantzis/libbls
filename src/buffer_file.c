/**
 * @file buffer_file.c
 *
 * Buffer file operations
 *
 * @author Alexandros Frantzis
 * @author Michael Iatrou
 */

#include <errno.h>
#include <stdlib.h>
#include "buffer.h"
#include "buffer_internal.h"
#include "segcol_list.h"

/**
 * Creates an empty bless_buffer_t.
 *
 * @param[out] buf an empty bless_buffer_t
 *
 * @return the operation error code
 */
int bless_buffer_new(bless_buffer_t **buf)
{
	if (buf == NULL)
		return EINVAL;

	*buf = malloc(sizeof **buf);

	if (*buf == NULL)
		return ENOMEM;
	
	int err = segcol_list_new(&(*buf)->segcol);
	if (err) {
		free(buf);
		return err;
	}

	return 0;
}

/**
 * Creates a bless_buffer_t from a file descriptor.
 *
 * The created bless_buffer_t initially contains data from
 * the specified file descriptor.
 *
 * @param[out] buf the created empty bless_buffer_t
 * @param fd the file descriptor.
 *
 * @return the operation error code
 */
int bless_buffer_new_from_file(bless_buffer_t **buf, int fd)
{
	return NULL;
}

/**
 * Saves the contents of a bless_buffer_t.
 *
 * The contents are saved in the file pointed to by the specified file
 * descriptor. 
 *
 * @param buf the bless_buffer_t whose contents to save
 * @param fd the file descriptor of the file to save the contents to
 * @param cb the bless_progress_cb to call to report the progress of the
 *           operation or NULL to disable reporting
 *
 * @return the operation error code
 */
int bless_buffer_save(bless_buffer_t *buf, int fd, bless_progress_cb cb)
{
	return -1;
}

/**
 * Frees a bless_buffer_t.
 *
 * Freeing a bless_buffer_t frees all
 * related resources but does not close any related files.
 *
 * @param buf the bless_buffer_t to close
 * @return the operation error code
 */
int bless_buffer_free(bless_buffer_t *buf)
{
	return -1;
}

