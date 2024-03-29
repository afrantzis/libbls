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
 * @file buffer_edit.c
 *
 * Buffer edit operations
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "buffer.h"
#include "buffer_internal.h"
#include "buffer_util.h"
#include "data_object.h"
#include "data_object_memory.h"
#include "type_limits.h"
#include "buffer_action.h"
#include "buffer_action_edit.h"
#include "buffer_event.h"
#include "util.h"
#include "debug.h"

#pragma GCC visibility push(default)

/********************
 * Helper functions *
 ********************/

/**
 * A segcol_foreach_func that reads data from a segment_t into memory.
 *
 * @param segcol the segcol_t containing the segment
 * @param seg the segment to read from
 * @param mapping the mapping of the segment in segcol
 * @param read_start the offset in the data of the segment to start reading
 * @param read_length the length of the data to read
 * @param user_data a void ** pointer containing the pointer to write to
 *
 * @return the operation error code
 */
static int read_foreach_func(segcol_t *segcol, segment_t *seg,
		off_t mapping, off_t read_start, off_t read_length, void *user_data)
{
	UNUSED_PARAM(segcol);
	UNUSED_PARAM(mapping);

	data_object_t *dobj;
	segment_get_data(seg, (void **)&dobj);

	/* user_data is actually a pointed to pointer */
	unsigned char **dst = (unsigned char **)user_data;

	int err = read_data_object(dobj, read_start, *dst, read_length);
	if (err)
		return_error(err);

	/* Move the pointer forwards */
	*dst += read_length;

	return 0;
}

/*****************
 * API Functions *
 *****************/

/**
 * Appends data to a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to append data to
 * @param src the source of the data
 * @param src_offset the offset of the data in the source
 * @param length the length of the data to append
 *
 * @return the operation error code
 */
int bless_buffer_append(bless_buffer_t *buf, bless_buffer_source_t *src,
		off_t src_offset, off_t length)
{
	if (buf == NULL || src == NULL) 
		return_error(EINVAL);

	buffer_action_t *action;
	struct bless_buffer_event_info event_info;

	/* Create an append action */
	int err = buffer_action_append_new(&action, buf, src, src_offset, length);
	if (err)
		return_error(err);

	/* Perform action */
	err = buffer_action_do(action);
	if (err)
		goto_error(err, on_error_do);
		
	/* 
	 * If we are in multi action mode, just add the action to the multi
	 * action and return.
	 */
	if (buf->multi_action_count) {
		/* We may not have an action if the undo limit is 0 */
		if (buf->multi_action != NULL) {
			err = buffer_action_multi_add(buf->multi_action, action);
			if (err)
				goto_error(err, on_error_other);
		}
		else {
			buf->first_rev_id = buf->next_rev_id++;
			buffer_action_free(action);
		}

		return 0;
	}

	/* Fill in the event info structure for this action */
	err = buffer_action_to_event(action, &event_info);
	if (err)
		goto_error(err, on_error_other);

	/* 
	 * Make sure that the undo list has space for one action (provided the
	 * undo limit is > 0).
	 */
	err = undo_list_enforce_limit(buf, 1);
	if (err)
		goto_error(err, on_error_other);

	/* 
	 * If we have space in the undo list to append the action.
	 * The only case we won't have space is when the undo limit is 0.
	 */
	if (buf->undo_list_size < buf->options->undo_limit) {
		err = undo_list_append(buf, action);
		if (err)
			goto_error(err, on_error_other);
	}
	else {
		buf->first_rev_id = buf->next_rev_id++;
		buffer_action_free(action);
	}

	action_list_clear(buf->redo_list);
	buf->redo_list_size = 0;

	/* Call event callback if supplied by the user */
	if (buf->event_func != NULL) {
		event_info.event_type = BLESS_BUFFER_EVENT_EDIT;
		(*buf->event_func)(buf, &event_info, buf->event_user_data);
	}

	return 0;

on_error_other:
	buffer_action_undo(action);
on_error_do:
	buffer_action_free(action);
	return err;
}

/**
 * Inserts data into a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to insert data into
 * @param offset the offset in the bless_buffer_t to insert data into
 * @param src the data source to insert data from
 * @param src_offset the offset in the source to insert data from
 * @param length the length of the data to insert
 *
 * @return the operation error code
 */
int bless_buffer_insert(bless_buffer_t *buf, off_t offset, 
		bless_buffer_source_t *src, off_t src_offset, off_t length)
{
	if (buf == NULL || src == NULL) 
		return_error(EINVAL);

	/* Create an insert action */
	buffer_action_t *action;
	struct bless_buffer_event_info event_info;

	int err = buffer_action_insert_new(&action, buf, offset, src, src_offset,
			length);
	if (err)
		return_error(err);

	/* Perform action */
	err = buffer_action_do(action);
	if (err)
		goto_error(err, on_error_do);

	/* 
	 * If we are in multi action mode, just add the action to the multi
	 * action and return.
	 */
	if (buf->multi_action_count) {
		/* We may not have an action if the undo limit is 0 */
		if (buf->multi_action != NULL) {
			err = buffer_action_multi_add(buf->multi_action, action);
			if (err)
				goto_error(err, on_error_other);
		}
		else {
			buf->first_rev_id = buf->next_rev_id++;
			buffer_action_free(action);
		}

		return 0;
	}

	/* Fill in the event info structure for this action */
	err = buffer_action_to_event(action, &event_info);
	if (err)
		goto_error(err, on_error_other);

	/* 
	 * Make sure that the undo list has space for one action (provided the
	 * undo limit is > 0).
	 */
	err = undo_list_enforce_limit(buf, 1);
	if (err)
		goto_error(err, on_error_other);

	/* 
	 * If we have space in the undo list to append the action.
	 * The only case we won't have space is when the undo limit is 0.
	 */
	if (buf->undo_list_size < buf->options->undo_limit) {
		err = undo_list_append(buf, action);
		if (err)
			goto_error(err, on_error_other);
	}
	else {
		buf->first_rev_id = buf->next_rev_id++;
		buffer_action_free(action);
	}

	action_list_clear(buf->redo_list);
	buf->redo_list_size = 0;

	/* Call event callback if supplied by the user */
	if (buf->event_func != NULL) {
		event_info.event_type = BLESS_BUFFER_EVENT_EDIT;
		(*buf->event_func)(buf, &event_info, buf->event_user_data);
	}

	return 0;

on_error_other:
	buffer_action_undo(action);
on_error_do:
	buffer_action_free(action);
	return err;
}

/**
 * Delete data from a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to delete data from
 * @param offset the offset in the bless_buffer_t to delete data from
 * @param length the length of the data to delete
 *
 * @return the operation error code
 */
int bless_buffer_delete(bless_buffer_t *buf, off_t offset, off_t length)
{
	if (buf == NULL)
		return_error(EINVAL);

	/* Create a delete action */
	buffer_action_t *action;
	struct bless_buffer_event_info event_info;

	int err = buffer_action_delete_new(&action, buf, offset, length);
	if (err)
		return_error(err);

	/* Perform action */
	err = buffer_action_do(action);
	if (err)
		goto_error(err, on_error_do);

	/* 
	 * If we are in multi action mode, just add the action to the multi
	 * action and return.
	 */
	if (buf->multi_action_count) {
		/* We may not have an action if the undo limit is 0 */
		if (buf->multi_action != NULL) {
			err = buffer_action_multi_add(buf->multi_action, action);
			if (err)
				goto_error(err, on_error_other);
		}
		else {
			buf->first_rev_id = buf->next_rev_id++;
			buffer_action_free(action);
		}

		return 0;
	}

	/* Fill in the event info structure for this action */
	err = buffer_action_to_event(action, &event_info);
	if (err)
		goto_error(err, on_error_other);

	/* 
	 * Make sure that the undo list has space for one action (provided the
	 * undo limit is > 0).
	 */
	err = undo_list_enforce_limit(buf, 1);
	if (err)
		goto_error(err, on_error_other);

	/* 
	 * If we have space in the undo list to append the action.
	 * The only case we won't have space is when the undo limit is 0.
	 */
	if (buf->undo_list_size < buf->options->undo_limit) {
		err = undo_list_append(buf, action);
		if (err)
			goto_error(err, on_error_other);
	}
	else {
		buf->first_rev_id = buf->next_rev_id++;
		buffer_action_free(action);
	}

	action_list_clear(buf->redo_list);
	buf->redo_list_size = 0;

	/* Call event callback if supplied by the user */
	if (buf->event_func != NULL) {
		event_info.event_type = BLESS_BUFFER_EVENT_EDIT;
		(*buf->event_func)(buf, &event_info, buf->event_user_data);
	}

	return 0;

on_error_other:
	buffer_action_undo(action);
on_error_do:
	buffer_action_free(action);
	return err;
}

/**
 * Reads data from a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to read data from
 * @param src_offset the offset in the bless_buffer_t to start reading from
 * @param dst a pointer to the start of the memory to store the data into
 * @param dst_offset the offset in dst to the start storing data into
 * @param length the length of the data to read
 *
 * @return the operation error code
 */
int bless_buffer_read(bless_buffer_t *buf, off_t src_offset, void *dst,
		size_t dst_offset, size_t length)
{
	if (buf == NULL || src_offset < 0 || dst == NULL) 
		return_error(EINVAL);

	/* 
	 * Check for dst overflow (src_offset overflow is checked in
	 * segcol_foreach()).
	 */
	if (__MAX(size_t) - (size_t)dst < dst_offset)
		return_error(EOVERFLOW);

	if (__MAX(size_t) - (size_t)dst - dst_offset < length - 1 * (length != 0))
		return_error(EOVERFLOW);

	void *cur_dst = (unsigned char *)dst + dst_offset;

	int err = segcol_foreach(buf->segcol, src_offset, length,
			read_foreach_func, &cur_dst);
	if (err)
		return_error(err);

	return 0;
}

/* bless_buffer_copy and bless_buffer_find are not implemented yet */
#pragma GCC visibility push(hidden)

/**
 * Copies data from a bless_buffer_t to another.
 *
 * @param src the bless_buffer_t to read data from
 * @param src_offset the offset in the source bless_buffer_t to start reading from
 * @param dst the bless_buffer_t to copy data to
 * @param dst_offset the offset in the destination bless_buffer_t to start copying data to
 * @param length the length of the data to read
 *
 * @return the operation error code
 */
int bless_buffer_copy(bless_buffer_t *src, off_t src_offset, bless_buffer_t *dst,
		off_t dst_offset, off_t length)
{
	UNUSED_PARAM(src);
	UNUSED_PARAM(src_offset);
	UNUSED_PARAM(dst);
	UNUSED_PARAM(dst_offset);
	UNUSED_PARAM(length);

	return_error(ENOSYS);
}

/**
 * Searches for data in a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to search
 * @param[out] match the offset the data was found at or -1 if no match was found
 * @param start_offset the offset in the bless_buffer_t to start searching from
 * @param data a pointer to the data to search for
 * @param length the length of the data to search for
 * @param progress_func the bless_progress_cb to call to report the progress of the
 *           operation or NULL to disable reporting
 *
 * @return the offset in the bless_buffer_t where a match was found or
 *         an operation error code
 */
int bless_buffer_find(bless_buffer_t *buf, off_t *match, off_t start_offset, 
		void *data, size_t length, bless_progress_func *progress_func)
{
	UNUSED_PARAM(buf);
	UNUSED_PARAM(match);
	UNUSED_PARAM(start_offset);
	UNUSED_PARAM(data);
	UNUSED_PARAM(length);
	UNUSED_PARAM(progress_func);

	return_error(ENOSYS);
}

#pragma GCC visibility pop

#pragma GCC visibility pop

