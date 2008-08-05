/**
 * @file buffer_info.c
 *
 * Buffer info operations
 *
 * @author Michael Iatrou
 * @author Alexandros Frantzis
 */

#include "buffer.h"
#include "buffer_internal.h"

/**
 * Checks whether the last operation in a bless_buffer_t can be undone.
 *
 * @param buf the bless_buffer_t to check
 *
 * @return 1 if the last operation can be undone, 0 otherwise
 */
int bless_buffer_can_undo(bless_buffer_t *buf)
{
	return -1;
}

/**
 * Checks whether the last undone operation in a bless_buffer_t can be redone.
 *
 * @param buf the bless_buffer_t to check
 *
 * @return 1 if the last undone operation can be redone, 0 otherwise
 */
int bless_buffer_can_redo(bless_buffer_t *buf)
{
	return -1;
}

/**
 * Gets the file descriptor associated with a bless_buffer_t.
 *
 * @param buf the bless_buffer_t
 *
 * @return the file descriptor
 */
int bless_buffer_get_fd(bless_buffer_t *buf)
{
	return NULL;
}

/**
 * Gets the size of a bless_buffer_t.
 *
 * @param buf the bless_buffer_t
 *
 * @return the size in bytes
 */
size_t bless_buffer_get_size(bless_buffer_t *buf)
{
	return -1;
}

