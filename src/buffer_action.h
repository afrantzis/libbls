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
 * Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file buffer_action.h
 *
 * Buffer actions API
 */
#ifndef _BUFFER_ACTION_H
#define _BUFFER_ACTION_H

#include "data_object.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup buffer_action Buffer Action
 *
 * A buffer action abstracts the notion of a doable and undoable actions
 * on the buffer.
 *
 * There are many kinds of buffer actions (eg append, insert). To create
 * a buffer action one must use the specific constructor function.
 *
 * @{
 */

/**
 * Opaque type for buffer action ADT.
 */
typedef struct buffer_action buffer_action_t;

int buffer_action_do(buffer_action_t *action);

int buffer_action_undo(buffer_action_t *action);

int buffer_action_private_copy(buffer_action_t *action, data_object_t *dobj);

int buffer_action_free(buffer_action_t *action);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _BUFFER_ACTION_H */

