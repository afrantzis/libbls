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
 * @file buffer_info.c
 *
 * Buffer info operations
 */

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "buffer.h"
#include "buffer_internal.h"

#include "util.h"


#pragma GCC visibility push(default)

/* bless_buffer_can_{undo,redo} are not implemented yet */
#pragma GCC visibility push(hidden)

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
	return_error(ENOSYS);
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
	return_error(ENOSYS);
}

#pragma GCC visibility pop

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
		return_error(EINVAL);

	return segcol_get_size(buf->segcol, size);
}

/** 
 * Sets a buffer option.
 * 
 * @param buf the buffer to set an option of
 * @param opt the option to set
 * @param val the value to set the option to 
 * 
 * @return the operation error code
 */
int bless_buffer_set_option(bless_buffer_t *buf, bless_buffer_option_t opt,
		char *val)
{
	if (buf == NULL || opt >= BLESS_BUF_SENTINEL)
		return_error(EINVAL);

	switch (opt) {
		case BLESS_BUF_TMP_DIR:
			if (val == NULL)
				buf->options->tmp_dir = NULL;
			else {
				char *dup = strdup(val);
				if (dup == NULL)
					return_error(ENOMEM);

				/* Free old value and set new one */
				if (buf->options->tmp_dir != NULL)
					free(buf->options->tmp_dir);
				buf->options->tmp_dir = dup;
			}
			break;

		default:
			break;
	}

	return 0;
}

/** 
 * Gets a buffer option.
 * 
 * The returned option value is the char * which is used internally
 * so it must not be altered.
 *
 * @param buf the buffer to get the option of
 * @param[out] val the returned value of the option
 * @param opt the option to get
 * 
 * @return the operation error code
 */
int bless_buffer_get_option(bless_buffer_t *buf, char **val,
		bless_buffer_option_t opt)
{
	if (buf == NULL || opt >= BLESS_BUF_SENTINEL)
		return_error(EINVAL);

	switch (opt) {
		case BLESS_BUF_TMP_DIR:
			*val = buf->options->tmp_dir;
			break;

		default:
			*val = NULL;
			break;
	}

	return 0;
}

#pragma GCC visibility pop

