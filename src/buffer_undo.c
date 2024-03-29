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
 * libbls.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file buffer_undo.c
 *
 * Buffer undo operations
 */

#include <errno.h>
#include <stdlib.h>
#include "buffer.h"
#include "buffer_util.h"
#include "buffer_internal.h"
#include "buffer_action.h"
#include "buffer_action_edit.h"
#include "debug.h"
#include "util.h"

#pragma GCC visibility push(default)

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
	struct list_node *last = list_tail(buf->undo_list)->prev;

	struct buffer_action_entry *entry = 
		list_entry(last, struct buffer_action_entry, ln);

	err = buffer_action_undo(entry->action);
	if (err)
		return_error(err);
	
	/* Remove the action from the undo list */
	err = list_delete_chain(last, last);
	if (err)
		goto_error(err, on_error_delete);

	--buf->undo_list_size;

	/* Add the entry to the redo list */
	err = list_insert_before(list_tail(buf->redo_list), &entry->ln);
	if (err)
		goto_error(err, on_error_insert);

	++buf->redo_list_size;

	/* Call event callback if supplied by the user */
	if (buf->event_func != NULL) {
		struct bless_buffer_event_info event_info;

		/* Fill in the event info structure for this action */
		err = buffer_action_to_event(entry->action, &event_info);
		if (err)
			return_error(err);

		event_info.event_type = BLESS_BUFFER_EVENT_UNDO;
		(*buf->event_func)(buf, &event_info, buf->event_user_data);
	}

	return 0;

on_error_insert:
	/* Add it back to the undo list */
	list_insert_before(list_tail(buf->undo_list), &entry->ln);
	++buf->undo_list_size;
on_error_delete:
	/* If we can't remove the action, redo it */
	buffer_action_do(entry->action);
	return err;

}


/**
 * Redoes the last undone operation in a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to redo the operation in
 *
 * @return the operation error code
 */
int bless_buffer_redo(bless_buffer_t *buf)
{
	if (buf == NULL)
		return_error(EINVAL);

	/* Make sure we can redo */
	int can_redo;
	int err = bless_buffer_can_redo(buf, &can_redo);
	if (err)
		return_error(err);

	if (!can_redo)
		return_error(EINVAL);

	/* Get the last action from the redo list and do it */
	struct list_node *last = list_tail(buf->redo_list)->prev;

	struct buffer_action_entry *entry = 
		list_entry(last, struct buffer_action_entry, ln);

	err = buffer_action_do(entry->action);
	if (err)
		return_error(err);
	
	/* Remove the action from the redo list */
	err = list_delete_chain(last, last);
	if (err)
		goto_error(err, on_error_delete);

	--buf->redo_list_size;

	/* 
	 * Add the entry to the undo list.
	 * We don't need to check if we can indeed move the entry to the undo list
	 * without surpassing the undo limit, because throughout the program we
	 * maintain the undo-redo invariant (undo+redo actions <= undo_limit).
	 */
	err = list_insert_before(list_tail(buf->undo_list), &entry->ln);
	if (err)
		goto_error(err, on_error_insert);

	++buf->undo_list_size;

	/* Call event callback if supplied by the user */
	if (buf->event_func != NULL) {
		struct bless_buffer_event_info event_info;

		/* Fill in the event info structure for this action */
		err = buffer_action_to_event(entry->action, &event_info);
		if (err)
			return_error(err);

		event_info.event_type = BLESS_BUFFER_EVENT_REDO;
		(*buf->event_func)(buf, &event_info, buf->event_user_data);
	}

	return 0;

on_error_insert:
	/* Add it back to the redo list */
	list_insert_before(list_tail(buf->redo_list), &entry->ln);
	++buf->redo_list_size;
on_error_delete:
	/* If we can't remove the action, undo it */
	buffer_action_undo(entry->action);
	return err;
}


/**
 * Marks the beginning of a multi-action.
 *
 * A multi-action is a compound action consisting of multiple
 * simple actions. In terms of undo-redo it is treated as
 * a single action.
 *
 * @param buf the bless_buffer_t on which following actions
 *            are to be treated as part of a single action
 * 
 * @return the operation error code
 */
int bless_buffer_begin_multi_action(bless_buffer_t *buf)
{
	if (buf == NULL)
		return_error(EINVAL);

	/* If we already are in multi-action mode, just update the count */
	if (buf->multi_action_count > 0) {
		buf->multi_action_count++;
		return 0;
	}

	/* 
	 * Make sure that the undo list has space for one action (provided the
	 * undo limit is > 0).
	 */
	int err = undo_list_enforce_limit(buf, 1);
	if (err)
		return_error(err);

	/* 
	 * If the undo limit is 0 we cannot append this action. Just change
	 * into multi_action mode and return.
	 */
	if (buf->undo_list_size >= buf->options->undo_limit) {
		buf->multi_action_count = 1;
		return 0;
	}

	/* Create the multi action that will hold the actions */
	err = buffer_action_multi_new(&buf->multi_action);
	if (err)
		return_error(err);

	err = undo_list_append(buf, buf->multi_action);
	if (err) {
		buffer_action_free(buf->multi_action);
		return_error(err);
	}

	action_list_clear(buf->redo_list);
	buf->redo_list_size = 0;

	/* We are now in multi action mode */
	buf->multi_action_count = 1;

	return 0;
}

/**
 * Marks the end of a multi-action.
 *
 * A multi-action is a compound action consisting of multiple
 * simple actions. In terms of undo-redo it is treated as
 * a single action.
 *
 * @param buf the bless_buffer_t on which following actions will
 *            stop being treated as part of a single action
 *
 * @return the operation error code
 */
int bless_buffer_end_multi_action(bless_buffer_t *buf)
{
	if (buf == NULL || buf->multi_action_count == 0)
		return_error(EINVAL);

	/* No need to do anything if we still have users */
	if (buf->multi_action_count > 1) {
		buf->multi_action_count--;
		return 0;
	}

	int err = 0;
	struct bless_buffer_event_info event_info;

	/* We may not have a multi_action if the undo limit is 0 */
	if (buf->multi_action != NULL) {
		/* Fill in the event info structure for this action */
		err = buffer_action_to_event(buf->multi_action, &event_info);
		if (err)
			return_error(err);
	}
	else {
		/* Create a dummy event info structure */
		event_info.action_type = BLESS_BUFFER_ACTION_MULTI;
		event_info.range_start = -1;
		event_info.range_length = -1;
		event_info.save_fd = -1;
	}
		
	/* Call event callback if supplied by the user */
	if (buf->event_func != NULL) {
		event_info.event_type = BLESS_BUFFER_EVENT_EDIT;
		(*buf->event_func)(buf, &event_info, buf->event_user_data);
	}

	/* We are now in normal action mode */
	buf->multi_action_count = 0;
	buf->multi_action = NULL;

	return 0;
}

#pragma GCC visibility pop
