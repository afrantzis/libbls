/*
 * Copyright 2009 Alexandros Frantzis, Michael Iatrou
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
 * @file buffer_action_edit.h
 *
 * Buffer edit actions API
 */
#ifndef _BUFFER_ACTION_EDIT_H
#define _BUFFER_ACTION_EDIT_H

#include "buffer.h"
#include "buffer_source.h"
#include "buffer_action.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup buffer_action
 *
 * @{
 */

/**
 * @name Constructor functions
 * 
 * @{
 */
int buffer_action_append_new(buffer_action_t **action, bless_buffer_t *buf,
		bless_buffer_source_t *src, off_t src_offset, off_t length);

int buffer_action_insert_new(buffer_action_t **action, bless_buffer_t *buf,
		off_t offset, bless_buffer_source_t *src, off_t src_offset, off_t length);

int buffer_action_delete_new(buffer_action_t **action, bless_buffer_t *buf,
		off_t offset, off_t length);

int buffer_action_multi_new(buffer_action_t **action);
int buffer_action_multi_add(buffer_action_t *multi_action,
		buffer_action_t *new_action);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _BUFFER_ACTION_EDIT_H */
