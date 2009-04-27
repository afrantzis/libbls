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
 * @file buffer_action.c
 *
 * Buffer actions implementation
 */
#include <stdlib.h>
#include <errno.h>

#include "data_object.h"
#include "buffer_action.h"
#include "buffer_action_internal.h"
#include "util.h"

struct buffer_action {
	void *impl;
	struct buffer_action_funcs *funcs;
};

/**********************
 * Internal functions *
 **********************/

/**
 * Creates a buffer_action_t using a specific implementation.
 *
 * @param[out] action the created buffer_action_t
 * @param impl the implementation private data
 * @param funcs function pointers to the implementations' functions
 *
 * @return the operation status code
 */
int buffer_action_create_impl(buffer_action_t **action, void *impl,
		struct buffer_action_funcs *funcs)
{
	if (action == NULL)
		return_error(EINVAL);

	*action = malloc(sizeof(buffer_action_t));

	if (*action == NULL)
		return_error(ENOMEM);

	(*action)->impl = impl;
	(*action)->funcs = funcs;

	return 0;
}

/**
 * Gets the implementation of a buffer_action_t
 *
 * @param action the buffer_action_t to get the implementation of
 *
 * @return the implementation
 */
void *buffer_action_get_impl(buffer_action_t *action)
{
	return action->impl;
}

/*****************
 * API functions *
 *****************/


/** 
 * Performs a buffer_action_t.
 * 
 * @param action the action to perform
 * 
 * @return the operation error code
 */
int buffer_action_do(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	return (*action->funcs->do_func)(action);
}

/** 
 * Reverts a buffer_action_t.
 * 
 * @param action the action to perform
 * 
 * @return the operation error code
 */
int buffer_action_undo(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	return (*action->funcs->undo_func)(action);
}

/** 
 * Makes a private copy of all the data held by this action
 * that belong to a data object.
 *
 * The data object is represented by a supplied
 * data_object_t although the original data may have been
 * accessed using another data_object_t (which of course
 * refers to the same data object as the supplied).
 * 
 * @param action the action to perform
 * @param dobj the data_object_t the data must belong to
 * 
 * @return the operation error code
 */
int buffer_action_private_copy(buffer_action_t *action, data_object_t *dobj)
{
	if (action == NULL || dobj == NULL)
		return_error(EINVAL);

	return (*action->funcs->private_copy_func)(action, dobj);
}

/** 
 * Frees a buffer_action_t.
 * 
 * @param action the action to perform
 * 
 * @return the operation error code
 */
int buffer_action_free(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	(*action->funcs->free_func)(action);
	free(action);
	return 0;
}
