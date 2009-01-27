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

#include <errno.h>
#include "buffer.h"
#include "buffer_internal.h"
#include "error.h"

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
	return BLESS_ENOTIMPL;
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
	return BLESS_ENOTIMPL;
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
		return EINVAL;

	return segcol_get_size(buf->segcol, size);
}

#pragma GCC visibility pop

