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
 * @file buffer_source.h
 *
 * Buffer source API
 */
#ifndef _BLESS_BUFFER_SOURCE_H
#define _BLESS_BUFFER_SOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/**
 * @addtogroup buffer
 *
 * @{
 */

/**
 * @name Buffer Sources
 */

/**
 * Opaque type for a buffer source object.
 */
typedef void bless_buffer_source_t;

/**
 * A function to call to free the memory associated with a memory source object.
 */
typedef void (bless_mem_free_func)(void *);

/**
 * A function to call to close the file associated with a file source object.
 */
typedef int (bless_file_close_func)(int);

int bless_buffer_source_memory(bless_buffer_source_t **src, void *data,
		size_t length, bless_mem_free_func *mem_free);

int bless_buffer_source_file(bless_buffer_source_t **src, int fd,
		bless_file_close_func *file_close);

int bless_buffer_source_unref(bless_buffer_source_t *src);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif
