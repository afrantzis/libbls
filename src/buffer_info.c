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
 * @file buffer_info.c
 *
 * Buffer info operations
 */

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "buffer.h"
#include "buffer_internal.h"
#include "buffer_util.h"
#include "buffer_event.h"
#include "type_limits.h"

#include "debug.h"


#pragma GCC visibility push(default)


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
	if (buf == NULL || can_undo == NULL)
		return_error(EINVAL);

	struct list_node *first = list_head(buf->undo_list)->next;

	*can_undo = !(first->next == first);

	return 0;
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
	if (buf == NULL || can_redo == NULL)
		return_error(EINVAL);

	struct list_node *first = list_head(buf->redo_list)->next;

	*can_redo = !(first->next == first);

	return 0;
}

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
 * Gets the revision id of the current buffer state.
 * 
 * @param buf the bless_buffer_t
 * @param[out] id the revision id
 * 
 * @return the operation error code
 */
int bless_buffer_get_revision_id(bless_buffer_t *buf, uint64_t *id)
{
	if (buf == NULL || id == NULL)
		return_error(EINVAL);

	uint64_t cur_id = buf->first_rev_id;

	if (buf->options->undo_limit > 0) {
		if (buf->undo_list_size > 0) {
			struct list_node *last = list_tail(buf->undo_list)->prev;
			struct buffer_action_entry *entry =
				list_entry(last, struct buffer_action_entry, ln);
			cur_id = entry->rev_id;	
		}
	}

	*id = cur_id;

	return 0;
}

/** 
 * Gets the revision id of the last saved buffer state.
 *
 * This is set automatically after a successful bless_buffer_save()
 * operation and can be manually set with bless_buffer_set_save_revision_id().
 * 
 * @param buf the bless_buffer_t
 * @param[out] id the revision id
 * 
 * @return the operation error code
 */
int bless_buffer_get_save_revision_id(bless_buffer_t *buf, uint64_t *id)
{
	if (buf == NULL || id == NULL)
		return_error(EINVAL);

	*id = buf->save_rev_id;

	return 0;
}

/** 
 * Sets the revision id of the last saved buffer state.
 * 
 * @param buf the bless_buffer_t
 * @param id the revision id
 * 
 * @return the operation error code
 */
int bless_buffer_set_save_revision_id(bless_buffer_t *buf, uint64_t id)
{
	if (buf == NULL)
		return_error(EINVAL);

	buf->save_rev_id = id;

	return 0;
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
				return_error(EINVAL);
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

		case BLESS_BUF_UNDO_LIMIT:
			if (val == NULL)
				return_error(EINVAL);
			else if (!strcmp(val, "infinite")) {
				char *dup = strdup(val);
				if (dup == NULL)
					return_error(ENOMEM);

				/* Free old value and set new one */
				if (buf->options->undo_limit_str != NULL)
					free(buf->options->undo_limit_str);

				buf->options->undo_limit_str = dup;
				buf->options->undo_limit = __MAX(size_t);
			}
			else {
				char *endptr;
				size_t limit = strtoul(val, &endptr, 10);
				if (*val == '\0' || *endptr != '\0')
					return_error(EINVAL);

				char *dup = strdup(val);
				if (dup == NULL)
					return_error(ENOMEM);

				/* Free old value and set new one */
				if (buf->options->undo_limit_str != NULL)
					free(buf->options->undo_limit_str);

				buf->options->undo_limit_str = dup;
				buf->options->undo_limit = limit;
			}

			/* 
			 * Make sure that the undo list size adheres to the new limit and
			 * clear the redo list.
			 */
			undo_list_enforce_limit(buf, 0);
			action_list_clear(buf->redo_list);
			buf->redo_list_size = 0;

			break;

		case BLESS_BUF_UNDO_AFTER_SAVE:
			if (val == NULL || (strcmp(val, "always") && strcmp(val, "never")
					&& strcmp(val, "best_effort")))
				return_error(EINVAL);
			else {
				char *dup = strdup(val);
				if (dup == NULL)
					return_error(ENOMEM);

				/* Free old value and set new one */
				if (buf->options->undo_after_save != NULL)
					free(buf->options->undo_after_save);
				buf->options->undo_after_save = dup;
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

		case BLESS_BUF_UNDO_LIMIT:
			*val = buf->options->undo_limit_str;
			break;

		case BLESS_BUF_UNDO_AFTER_SAVE:
			*val = buf->options->undo_after_save;
			break;

		default:
			*val = NULL;
			break;
	}

	return 0;
}

/** 
 * Sets the callback function used to report buffer events.
 *
 * The callback function can be set to NULL to disable event
 * reporting.
 * 
 * @param buf the bless_buffer_t to set the callback function of
 * @param func the function to set
 * @param user_data user data to be supplied to the callback function
 * 
 * @return the operation error code
 */
int bless_buffer_set_event_callback(bless_buffer_t *buf,
		bless_buffer_event_func_t *func, void *user_data)
{
	if (buf == NULL)
		return_error(EINVAL);

	buf->event_func = func;
	buf->event_user_data = user_data;

	return 0;
}

/** 
 * Gets the current multi action usage count.
 * 
 * @param buf the bless_buffer_t
 * @param[out] multi the multi action usage count
 * 
 * @return the operation error code
 */
int bless_buffer_query_multi_action(bless_buffer_t *buf, int *multi)
{
	if (buf == NULL || multi == NULL)
		return_error(EINVAL);

	*multi = buf->multi_action_count;

	return 0;
}

#pragma GCC visibility pop

