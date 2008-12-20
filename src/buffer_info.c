/**
 * @file buffer_info.c
 *
 * Buffer info operations
 *
 * @author Michael Iatrou
 * @author Alexandros Frantzis
 */

#include <errno.h>
#include "buffer.h"
#include "buffer_internal.h"

/**
 * Checks whether the last operation in a bless_buffer_t can be undone.
 *
 * @param buf the bless_buffer_t to check
 * @param[out] can_undo 1 if the last operation can be undone, 0 otherwise
 *
 * @return the operation error code
 */
int bless_buffer_can_undo(bless_buffer_t *buf, int *can_undo)
{
	return -1;
}

/**
 * Checks whether the last undone operation in a bless_buffer_t can be redone.
 *
 * @param buf the bless_buffer_t to check
 * @param[out] can_redo 1 if the last undone operation can be undone, 0 otherwise
 *
 * @return the operation error code
 */
int bless_buffer_can_redo(bless_buffer_t *buf, int *can_redo)
{
	return -1;
}

/**
 * Gets the size of a bless_buffer_t.
 *
 * @param buf the bless_buffer_t
 * @param[out] size the size in bytes
 *
 * @return the operation error code
 */
int bless_buffer_get_size(bless_buffer_t *buf, off_t *size)
{
	if (buf == NULL || size == NULL)
		return EINVAL;

	return segcol_get_size(buf->segcol, size);
}

