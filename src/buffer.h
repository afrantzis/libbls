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
 * @file buffer.h
 *
 * Public buffer API.
 */
#ifndef _BLESS_BUFFER_H
#define _BLESS_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>


#include "buffer_source.h"
#include "buffer_options.h"

/**
 * @defgroup buffer Buffer
 *
 * @{
 */

/**
 * Opaque data type for a bless buffer.
 */
typedef struct bless_buffer bless_buffer_t;

/** 
 * Callback function called to report the progress of long operations.
 *
 * This callback is used by operations that may take a long time to finish.
 * These operations call the callback periodically and pass progress info using
 * the info argument. The return value of the callback function is checked by
 * the operations to decide whether they should continue.
 *
 * @param info operation specific progress info
 *
 * @return 1 if the operation must be cancelled, 0 otherwise
 */
typedef int (bless_progress_func)(void *info);

/**
 * @name File Operations
 *
 * @{
 */

int bless_buffer_new(bless_buffer_t **buf);

int bless_buffer_save(bless_buffer_t *buf, int fd,
		bless_progress_func *progress_func);

int bless_buffer_free(bless_buffer_t *buf);

/** @} */
/**
 * @name Edit Operations
 *
 * @{
 */

int bless_buffer_append(bless_buffer_t *buf, bless_buffer_source_t *src,
		off_t src_offset, off_t length);

int bless_buffer_insert(bless_buffer_t *buf, off_t offset, 
		bless_buffer_source_t *src, off_t src_offset, off_t length);

int bless_buffer_delete(bless_buffer_t *buf, off_t offset, off_t length);

int bless_buffer_read(bless_buffer_t *src, off_t src_offset, void *dst,
		size_t dst_offset, size_t length);

/* Not yet implemented
int bless_buffer_copy(bless_buffer_t *src, off_t src_offset, bless_buffer_t *dst,
		off_t dst_offset, off_t length);

int bless_buffer_find(bless_buffer_t *buf, off_t *match, off_t start_offset, 
		void *data, size_t length, bless_progress_func *progress_func);
*/

/** @} */
/**
 * @name Undo - Redo Operations
 *
 * @{
 */

int bless_buffer_undo(bless_buffer_t *buf);

int bless_buffer_redo(bless_buffer_t *buf);

/* Not yet implemented
int bless_buffer_begin_multi_op(bless_buffer_t *buf);

int bless_buffer_end_multi_op(bless_buffer_t *buf);
*/

/** @} */
/**
 * @name Buffer Information/Options
 *
 * @{
 */

int bless_buffer_can_undo(bless_buffer_t *buf, int *can_undo);

int bless_buffer_can_redo(bless_buffer_t *buf, int *can_redo);

int bless_buffer_get_size(bless_buffer_t *buf, off_t *size);

int bless_buffer_set_option(bless_buffer_t *buf, bless_buffer_option_t opt,
		char *val);

int bless_buffer_get_option(bless_buffer_t *buf, char **val,
		bless_buffer_option_t opt);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _BLESS_BUFFER_H */
