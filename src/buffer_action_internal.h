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
 * @file buffer_action_internal.h
 *
 * Buffer actions internal API
 */
#ifndef _BUFFER_ACTION_INTERNAL_H
#define _BUFFER_ACTION_INTERNAL_H

#include "data_object.h"
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
 * Struct that holds the function pointers for an implementation of a
 * buffer_action_t.
 */
struct buffer_action_funcs {
	int (*do_func)(buffer_action_t *action);
	int (*undo_func)(buffer_action_t *action);
	int (*private_copy_func)(buffer_action_t *action, data_object_t *obj);
	int (*to_event_func)(buffer_action_t *action,
			struct bless_buffer_event_info *event_info);
	int (*free_func)(buffer_action_t *action);
};

/**
 * @name Internal functions
 * 
 * @{
 */
int buffer_action_create_impl(buffer_action_t **action, void *impl,
		struct buffer_action_funcs *funcs);

void *buffer_action_get_impl(buffer_action_t *action);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _BUFFER_ACTION_INTERNAL_H */
