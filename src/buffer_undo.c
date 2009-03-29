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
 * @file buffer_undo.c
 *
 * Buffer undo operations
 */

#include <errno.h>
#include <stdlib.h>
#include "buffer.h"
#include "buffer_internal.h"
#include "buffer_action.h"
#include "util.h"


/**
 * Undoes the last operation in a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to undo the operation in
 *
 * @return the operation error code
 */
int bless_buffer_undo(bless_buffer_t *buf)
{
	if (buf == NULL)
		return_error(EINVAL);

	/* Make sure we can undo */
	int can_undo;
	int err = bless_buffer_can_undo(buf, &can_undo);
	if (err)
		return_error(err);

	if (!can_undo)
		return_error(EINVAL);

	/* Get the last action from the undo list and undo it */
	struct list_node *last = action_list_tail(buf->undo_list)->prev;

	struct buffer_action_entry *entry = 
		list_entry(last, struct buffer_action_entry, ln);

	err = buffer_action_undo(entry->action);
	if (err)
		return_error(err);
	
	/* Remove the action from the action list */
	err = list_delete_chain(last, last);
	if (err) {
		/* If we can remove the action redo it and report error */
		buffer_action_do(entry->action);
		return_error(err);
	}

	/* Free the action and the list entry */
	buffer_action_free(entry->action);
	free(entry);

	return 0;
}

#pragma GCC visibility push(hidden)

/**
 * Redoes the last undone operation in a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to redo the operation in
 *
 * @return the operation error code
 */
int bless_buffer_redo(bless_buffer_t *buf)
{
	return_error(ENOSYS);
}

/**
 * Marks the beginning of a multi-op.
 *
 * A multi-op is a compound operation consisting of multiple
 * simple operations. In terms of undo-redo it is treated as
 * a single operation.
 *
 * @param buf the bless_buffer_t on which following operations
 *            are to be treated as part of a single operation
 * 
 * @return the operation error code
 */
int bless_buffer_begin_multi_op(bless_buffer_t *buf)
{
	return_error(ENOSYS);
}

/**
 * Marks the end of a multi-op.
 *
 * A multi-op is a compound operation consisting of multiple
 * simple operations. In terms of undo-redo it is treated as
 * a single operation.
 *
 * @param buf the bless_buffer_t on which following operations will
 *            stop to be treated as part of a single operation
 *
 * @return the operation error code
 */
int bless_buffer_end_multi_op(bless_buffer_t *buf)
{
	return_error(ENOSYS);
}

#pragma GCC visibility pop

