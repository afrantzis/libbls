/*
 * Copyright 2009 Alexandros Frantzis, Michael Iatrou
 *
 * This file is part of libbls.
 *
 * libbls is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
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
 * Buffer edit actions implementation
 */
#include <stdlib.h>
#include <errno.h>

#include "buffer.h"
#include "buffer_internal.h"
#include "buffer_source.h"
#include "buffer_action.h"
#include "buffer_action_internal.h"
#include "buffer_action_edit.h"
#include "buffer_util.h"
#include "segcol.h"
#include "data_object.h"
#include "data_object_memory.h"
#include "debug.h"


/* Forward declarations */

/* Helper functions */
static int create_segment_from_source(segment_t **seg, 
		bless_buffer_source_t *src, off_t src_offset, off_t length);
static int segment_inplace_private_copy(segment_t *seg, data_object_t *cmp_dobj);
static int segcol_inplace_private_copy(segcol_t *segcol, data_object_t *cmp_dobj);

/* API functions */
static int buffer_action_append_do(buffer_action_t *action);
static int buffer_action_append_undo(buffer_action_t *action);
static int buffer_action_append_private_copy(buffer_action_t *action,
		data_object_t *dobj);
static int buffer_action_append_to_event(buffer_action_t *action,
		struct bless_buffer_event_info *event_info);
static int buffer_action_append_free(buffer_action_t *action);


static int buffer_action_insert_do(buffer_action_t *action);
static int buffer_action_insert_undo(buffer_action_t *action);
static int buffer_action_insert_private_copy(buffer_action_t *action,
		data_object_t *dobj);
static int buffer_action_insert_to_event(buffer_action_t *action,
		struct bless_buffer_event_info *event_info);
static int buffer_action_insert_free(buffer_action_t *action);

static int buffer_action_delete_do(buffer_action_t *action);
static int buffer_action_delete_undo(buffer_action_t *action);
static int buffer_action_delete_private_copy(buffer_action_t *action,
		data_object_t *dobj);
static int buffer_action_delete_to_event(buffer_action_t *action,
		struct bless_buffer_event_info *event_info);
static int buffer_action_delete_free(buffer_action_t *action);

static int buffer_action_multi_do(buffer_action_t *action);
static int buffer_action_multi_undo(buffer_action_t *action);
static int buffer_action_multi_private_copy(buffer_action_t *action,
		data_object_t *dobj);
static int buffer_action_multi_to_event(buffer_action_t *action,
		struct bless_buffer_event_info *event_info);
static int buffer_action_multi_free(buffer_action_t *action);

/* Action functions */
static struct buffer_action_funcs buffer_action_append_funcs = {
	.do_func = buffer_action_append_do,
	.undo_func = buffer_action_append_undo,
	.private_copy_func = buffer_action_append_private_copy,
	.to_event_func = buffer_action_append_to_event,
	.free_func = buffer_action_append_free
};

static struct buffer_action_funcs buffer_action_insert_funcs = {
	.do_func = buffer_action_insert_do,
	.undo_func = buffer_action_insert_undo,
	.private_copy_func = buffer_action_insert_private_copy,
	.to_event_func = buffer_action_insert_to_event,
	.free_func = buffer_action_insert_free
};

static struct buffer_action_funcs buffer_action_delete_funcs = {
	.do_func = buffer_action_delete_do,
	.undo_func = buffer_action_delete_undo,
	.private_copy_func = buffer_action_delete_private_copy,
	.to_event_func = buffer_action_delete_to_event,
	.free_func = buffer_action_delete_free
};

static struct buffer_action_funcs buffer_action_multi_funcs = {
	.do_func = buffer_action_multi_do,
	.undo_func = buffer_action_multi_undo,
	.private_copy_func = buffer_action_multi_private_copy,
	.to_event_func = buffer_action_multi_to_event,
	.free_func = buffer_action_multi_free
};

/* Action implementations */
struct buffer_action_append_impl {
	bless_buffer_t *buf;
	segment_t *seg;
};

struct buffer_action_insert_impl {
	bless_buffer_t *buf;
	off_t offset;
	segment_t *seg;
};

struct buffer_action_delete_impl {
	bless_buffer_t *buf;
	off_t offset;
	off_t length;
	segcol_t *deleted;
};

struct buffer_action_multi_impl {
	bless_buffer_t *buf;
	list_t *action_list;
};

/********************
 * Helper functions *
 ********************/

/**
 * Create a segment from a data_object_t.
 *
 * @param[out] seg the created segment
 * @param src_dobj the data_object_t
 * @param src_offset the start of the segment range in src
 * @param length the length of the segment range
 *
 * @return the operation error code
 */
static int create_segment_from_source(segment_t **seg, 
		bless_buffer_source_t *src, off_t src_offset, off_t length)
{
	data_object_t *dobj = (data_object_t *) src;
	/* Create a segment pointing to the data object */
	int err = segment_new(seg, dobj, src_offset, length,
			data_object_update_usage);
	if (err)
		return_error(err);

	/* 
	 * Check if the specified file range is valid. This is done 
	 * here so that overflow has already been checked by segment_new().
	 */
	off_t dobj_size;
	err = data_object_get_size(dobj, &dobj_size);
	if (err)
		goto_error(err, on_error);

	if (src_offset + length - 1 * (length != 0) >= dobj_size) {
		err = EINVAL;
		goto_error(err, on_error);
	}

	return 0;

on_error:
	/* No need to free obj, this is handled by segment_free */
	segment_free(*seg);
	return err;
}

/** 
 * Makes a private copy of the data held by a segment if
 * that data belongs to a specific data_object_t.
 *
 * The operation is performed in-place: upon completion
 * the segment will point to the private copy of the data.
 * 
 * @param seg the segment_t
 * @param cmp_dobj the data_object_t the data must belong to
 * 
 * @return the operation error code
 */
static int segment_inplace_private_copy(segment_t *seg, data_object_t *cmp_dobj)
{
	data_object_t *seg_dobj;
	off_t seg_start;
	off_t seg_size;

	/* Get segment information */
	int err = segment_get_data(seg, (void **)&seg_dobj);
	if (err)
		return_error(err);

	/* Continue this operation only if the data comes from the cmp_dobj */
	int result;
	err = data_object_compare(&result, seg_dobj, cmp_dobj);
	if (err)
		return_error(err);
	if (result != 0)
		return 0;

	err = segment_get_start(seg, &seg_start);
	if (err)
		return_error(err);

	err = segment_get_size(seg, &seg_size);
	if (err)
		return_error(err);

	/* Create new data object to hold data */
	void *new_data = malloc(seg_size);
	if (new_data == NULL)
		return_error(ENOMEM);

	data_object_t *new_dobj;
	err = data_object_memory_new(&new_dobj, new_data, seg_size);
	if (err)
		goto_error(err, on_error_new);

	/* Set the data object's free function */
	err = data_object_memory_set_free_func(new_dobj, free);
	if (err)
		goto_error(err, on_error_set_free_func);

	/* Copy the data to the new object and setup the segment */
	err = read_data_object(seg_dobj, seg_start, new_data, seg_size);
	new_data = NULL;
	if (err)
		goto_error(err, on_error_read);

	err = segment_set_range(seg, 0, seg_size);
	if (err)
		goto_error(err, on_error_set_range);

	err = segment_set_data(seg, new_dobj, data_object_update_usage);
	if (err)
		goto_error(err, on_error_set_data);

	return 0;

on_error_set_data:
	segment_set_range(seg, seg_start, seg_size);
on_error_set_range:
on_error_read:
on_error_set_free_func:
	data_object_free(new_dobj);
on_error_new:
	if (new_data != NULL)
		free(new_data);

	return err;
}

/** 
 * Makes a private copy of the data held by a segcol if
 * that data belongs to a specific data_object_t.
 *
 * The operation is performed in-place: upon completion
 * the segments in the segment collection will point to
 * the private copy of the data.
 * 
 * @param seg the segcol_t
 * @param cmp_dobj the data_object_t the data must belong to
 * 
 * @return the operation error code
 */
static int segcol_inplace_private_copy(segcol_t *segcol, data_object_t *cmp_dobj)
{
	segcol_iter_t *iter;

	int err = segcol_iter_new(segcol, &iter);
	if (err)
		return_error(err);

	int iter_valid;

	/* 
	 * Iterate over the whole segcol
	 */
	while (!(err = segcol_iter_is_valid(iter, &iter_valid)) && iter_valid) {
		segment_t *seg;

		err = segcol_iter_get_segment(iter, &seg);
		if (err)
			goto_error(err, out);

		/* Make private copy of segment data (if they belong to cmp_dobj) */
		err = segment_inplace_private_copy(seg, cmp_dobj);
		if (err)
			goto_error(err, out);

		err = segcol_iter_next(iter);
		if (err)
			goto_error(err, out);
	}

out:
	segcol_iter_free(iter);
	return err;
}

/****************
 * Constructors *
 ****************/

/** 
 * Creates a new append buffer_action_t.
 * 
 * @param [out] action the created buffer_action_t
 * @param buf the buffer_t to append data to
 * @param src the source to append data from
 * @param src_offset the offset of the source to get data from
 * @param length the length in bytes of the data to append
 * 
 * @return the operation error code
 */
int buffer_action_append_new(buffer_action_t **action, bless_buffer_t *buf,
		bless_buffer_source_t *src, off_t src_offset, off_t length)
{
	if (action == NULL || buf == NULL || src == NULL)
		return_error(EINVAL);

	/* Allocate memory for implementation */
	struct buffer_action_append_impl *impl =
		malloc(sizeof(struct buffer_action_append_impl));
	
	if (impl == NULL)
		return_error(EINVAL);

	/* Create buffer_action_t */
	int err = buffer_action_create_impl(action, impl,
			&buffer_action_append_funcs);
	if (err)
		goto_error(err, on_error_impl);

	/* Initialize implementation */
	err = create_segment_from_source(&impl->seg, src, src_offset, length);
	if (err)
		goto_error(err, on_error_segment);

	impl->buf = buf;

	return 0;

on_error_segment:
	free(*action);
on_error_impl:
	free(impl);
	return err;
}

/** 
 * Creates a new insert buffer_action_t.
 * 
 * @param [out] action the created buffer_action_t
 * @param buf the buffer_t to insert data to
 * @param offset the offset in the buffer_t to insert to
 * @param src the source to append data from
 * @param src_offset the offset of the source to get data from
 * @param length the length in bytes of the data to append
 * 
 * @return the operation error code
 */
int buffer_action_insert_new(buffer_action_t **action, bless_buffer_t *buf,
		off_t offset, bless_buffer_source_t *src, off_t src_offset, off_t length)
{
	if (action == NULL || buf == NULL || src == NULL)
		return_error(EINVAL);

	/* Allocate memory for implementation */
	struct buffer_action_insert_impl *impl =
		malloc(sizeof(struct buffer_action_insert_impl));
	
	if (impl == NULL)
		return_error(EINVAL);

	/* Create buffer_action_t */
	int err = buffer_action_create_impl(action, impl,
			&buffer_action_insert_funcs);
	if (err)
		goto_error(err, on_error_impl);

	/* Initialize implementation */
	err = create_segment_from_source(&impl->seg, src, src_offset, length);
	if (err)
		goto_error(err, on_error_segment);

	impl->buf = buf;
	impl->offset = offset;

	return 0;

on_error_segment:
	free(*action);
on_error_impl:
	free(impl);
	return err;
}

/** 
 * Creates a new delete buffer_action_t.
 * 
 * @param [out] action the created buffer_action_t
 * @param buf the buffer_t to delete data from
 * @param offset the offset in the buffer_t to delete from
 * @param length the length in bytes of the data to delete
 * 
 * @return the operation error code
 */
int buffer_action_delete_new(buffer_action_t **action, bless_buffer_t *buf,
		off_t offset, off_t length)
{
	if (action == NULL || buf == NULL)
		return_error(EINVAL);

	/* Allocate memory for implementation */
	struct buffer_action_delete_impl *impl =
		malloc(sizeof(struct buffer_action_delete_impl));
	
	if (impl == NULL)
		return_error(EINVAL);

	/* Create buffer_action_t */
	int err = buffer_action_create_impl(action, impl,
			&buffer_action_delete_funcs);
	if (err)
		goto_error(err, on_error);

	/* Initialize implementation */
	impl->buf = buf;
	impl->offset = offset;
	impl->length = length;
	impl->deleted = NULL;

	return 0;

on_error:
	free(impl);
	return err;
}

/** 
 * Creates a new multi buffer_action_t.
 * 
 * @param [out] action the created buffer_action_t
 * 
 * @return the operation error code
 */
int buffer_action_multi_new(buffer_action_t **action)
{
	if (action == NULL)
		return_error(EINVAL);

	/* Allocate memory for implementation */
	struct buffer_action_multi_impl *impl =
		malloc(sizeof(struct buffer_action_multi_impl));
	
	if (impl == NULL)
		return_error(EINVAL);

	/* Create buffer_action_t */
	int err = buffer_action_create_impl(action, impl,
			&buffer_action_multi_funcs);
	if (err)
		goto_error(err, on_error);

	/* Initialize implementation */
	err = list_new(&impl->action_list, struct buffer_action_entry, ln);
	if (err)
		goto_error(err, on_error);

	return 0;

on_error:
	free(impl);
	return err;
}

/** 
 * Adds a new action to a multi action.
 * 
 * @param multi_action the multi action to add an action to
 * @param new_action the new action to add
 * 
 * @return the operation error code
 */
int buffer_action_multi_add(buffer_action_t *multi_action, buffer_action_t *new_action)
{
	if (multi_action == NULL || new_action == NULL)
		return_error(EINVAL);

	struct buffer_action_multi_impl *impl =
		(struct buffer_action_multi_impl *) buffer_action_get_impl(multi_action);

	/* Create entry to hold new action */
	struct buffer_action_entry *entry =
		malloc(sizeof(struct buffer_action_entry));
	
	if (entry == NULL)
		return_error(EINVAL);

	entry->action = new_action;

	/* Append entry to multi action list */
	int err = list_insert_before(list_tail(impl->action_list), &entry->ln);
	if (err) {
		free(entry);
		return_error(err);
	}

	return 0;
}

/********************
 * Append Functions *
 ********************/

static int buffer_action_append_do(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	struct buffer_action_append_impl *impl =
		(struct buffer_action_append_impl *) buffer_action_get_impl(action);

	/* 
	 * No need to check for overflow, because it is detected by the
	 * functions that follow.
	 */
	segment_t *seg;

	int err = segment_copy(impl->seg, &seg);
	if (err)
		return_error(err);
	
	segcol_t *sc = impl->buf->segcol;

	/* Append segment to the segcol */
	err = segcol_append(sc, seg);
	if (err) {
		segment_free(seg);
		return_error(err);
	}

	return 0;
}

static int buffer_action_append_undo(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	struct buffer_action_append_impl *impl =
		(struct buffer_action_append_impl *) buffer_action_get_impl(action);

	/* 
	 * No need to check for overflow, because it is detected by the
	 * functions that follow.
	 */
	segcol_t *sc = impl->buf->segcol;
	off_t segcol_size;
	int err = segcol_get_size(sc, &segcol_size);
	if (err)
		return_error(err);

	off_t seg_size;
	err = segment_get_size(impl->seg, &seg_size);
	if (err)
		return_error(err);

	/* Delete range from the segcol */
	err = segcol_delete(sc, NULL, segcol_size - seg_size, seg_size);
	if (err)
		return_error(err);

	return 0;
}

static int buffer_action_append_private_copy(buffer_action_t *action,
		data_object_t *cmp_dobj)
{
	if (action == NULL || cmp_dobj == NULL)
		return_error(EINVAL);

	struct buffer_action_append_impl *impl =
		(struct buffer_action_append_impl *) buffer_action_get_impl(action);

	int err = segment_inplace_private_copy(impl->seg, cmp_dobj);
	if (err)
		return_error(err);

	return 0;
}

static int buffer_action_append_to_event(buffer_action_t *action,
		struct bless_buffer_event_info *event_info)
{
	if (action == NULL || event_info == NULL)
		return_error(EINVAL);

	struct buffer_action_append_impl *impl =
		(struct buffer_action_append_impl *) buffer_action_get_impl(action);

	off_t buf_size;
	bless_buffer_get_size(impl->buf, &buf_size);

	off_t seg_size;
	segment_get_size(impl->seg, &seg_size);

	event_info->action_type = BLESS_BUFFER_ACTION_APPEND;
	event_info->range_start = buf_size - seg_size;
	event_info->range_length = seg_size;
	event_info->save_fd = -1;

	return 0;
}

static int buffer_action_append_free(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	struct buffer_action_append_impl *impl =
		(struct buffer_action_append_impl *) buffer_action_get_impl(action);

	int err = segment_free(impl->seg);
	if (err)
		return_error(err);

	free(impl);

	return 0;
}

/********************
 * Insert Functions *
 ********************/

static int buffer_action_insert_do(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	struct buffer_action_insert_impl *impl =
		(struct buffer_action_insert_impl *) buffer_action_get_impl(action);

	/* 
	 * No need to check for overflow, because it is detected by the
	 * functions that follow.
	 */
	segment_t *seg;

	int err = segment_copy(impl->seg, &seg);
	if (err)
		return_error(err);
	
	segcol_t *sc = impl->buf->segcol;

	/* Append segment to the segcol */
	err = segcol_insert(sc, impl->offset, seg);
	if (err) {
		segment_free(seg);
		return_error(err);
	}

	return 0;
}

static int buffer_action_insert_undo(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	struct buffer_action_insert_impl *impl =
		(struct buffer_action_insert_impl *) buffer_action_get_impl(action);

	/* 
	 * No need to check for overflow, because it is detected by the
	 * functions that follow.
	 */
	segcol_t *sc = impl->buf->segcol;

	off_t seg_size;
	int err = segment_get_size(impl->seg, &seg_size);
	if (err)
		return_error(err);

	/* Delete range from the segcol */
	err = segcol_delete(sc, NULL, impl->offset, seg_size);
	if (err)
		return_error(err);

	return 0;
}

static int buffer_action_insert_private_copy(buffer_action_t *action,
		data_object_t *cmp_dobj)
{
	if (action == NULL || cmp_dobj == NULL)
		return_error(EINVAL);

	struct buffer_action_insert_impl *impl =
		(struct buffer_action_insert_impl *) buffer_action_get_impl(action);

	int err = segment_inplace_private_copy(impl->seg, cmp_dobj);
	if (err)
		return_error(err);

	return 0;
}

static int buffer_action_insert_to_event(buffer_action_t *action,
		struct bless_buffer_event_info *event_info)
{
	if (action == NULL || event_info == NULL)
		return_error(EINVAL);

	struct buffer_action_insert_impl *impl =
		(struct buffer_action_insert_impl *) buffer_action_get_impl(action);

	off_t seg_size;
	segment_get_size(impl->seg, &seg_size);

	event_info->action_type = BLESS_BUFFER_ACTION_INSERT;
	event_info->range_start = impl->offset;
	event_info->range_length = seg_size;
	event_info->save_fd = -1;

	return 0;
}

static int buffer_action_insert_free(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	struct buffer_action_insert_impl *impl =
		(struct buffer_action_insert_impl *) buffer_action_get_impl(action);

	int err = segment_free(impl->seg);
	if (err)
		return_error(err);

	free(impl);

	return 0;
}

/********************
 * Delete Functions *
 ********************/

static int buffer_action_delete_do(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	struct buffer_action_delete_impl *impl =
		(struct buffer_action_delete_impl *) buffer_action_get_impl(action);

	/* 
	 * No need to check for overflow, valid ranges etc.
	 * They are all checked in segcol_delete().
	 */
	segcol_t *deleted;
	int err = segcol_delete(impl->buf->segcol, &deleted, impl->offset,
			impl->length);
	if (err)
		return_error(err);

	/* Free previous deleted data if any */
	if (impl->deleted != NULL) {
		err = segcol_free(impl->deleted);
		if (err) {
			segcol_add_copy(impl->buf->segcol, impl->offset, deleted);
			segcol_free(deleted);
			return_error(err);
		}
	}

	/* Store new deleted data */
	impl->deleted = deleted;

	return 0;
}

static int buffer_action_delete_undo(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	struct buffer_action_delete_impl *impl =
		(struct buffer_action_delete_impl *) buffer_action_get_impl(action);

	/* Add the deleted data back to the segcol */
	int err = segcol_add_copy(impl->buf->segcol, impl->offset, impl->deleted);
	if (err)
		return_error(err);
		
	return 0;
}

static int buffer_action_delete_private_copy(buffer_action_t *action,
		data_object_t *cmp_dobj)
{
	if (action == NULL || cmp_dobj == NULL)
		return_error(EINVAL);

	struct buffer_action_delete_impl *impl =
		(struct buffer_action_delete_impl *) buffer_action_get_impl(action);

	int err = segcol_inplace_private_copy(impl->deleted, cmp_dobj);
	if (err)
		return_error(err);

	return 0;
}

static int buffer_action_delete_to_event(buffer_action_t *action,
		struct bless_buffer_event_info *event_info)
{
	if (action == NULL || event_info == NULL)
		return_error(EINVAL);

	struct buffer_action_delete_impl *impl =
		(struct buffer_action_delete_impl *) buffer_action_get_impl(action);

	event_info->action_type = BLESS_BUFFER_ACTION_DELETE;
	event_info->range_start = impl->offset;
	event_info->range_length = impl->length;
	event_info->save_fd = -1;

	return 0;
}

static int buffer_action_delete_free(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	struct buffer_action_delete_impl *impl =
		(struct buffer_action_delete_impl *) buffer_action_get_impl(action);

	if (impl->deleted != NULL) {
		int err = segcol_free(impl->deleted);
		if (err)
			return_error(err);
	}

	free(impl);

	return 0;
}

/*******************
 * Multi Functions *
 *******************/

static int buffer_action_multi_do(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	int err = 0;

	struct buffer_action_multi_impl *impl =
		(struct buffer_action_multi_impl *) buffer_action_get_impl(action);

	struct list_node *node;
	list_for_each(list_head(impl->action_list)->next, node) {
		struct buffer_action_entry *entry =
			list_entry(node, struct buffer_action_entry, ln);

		buffer_action_t *a = entry->action;

		err = buffer_action_do(a);
		if (err)
			break;
	}

	if (err) {
		struct list_node *start = node->prev;
		list_for_each_reverse(start, node) {
			struct buffer_action_entry *entry =
				list_entry(node, struct buffer_action_entry, ln);

			buffer_action_t *a = entry->action;

			buffer_action_undo(a);
		}
		return_error(err);
	}

	return 0;
}

static int buffer_action_multi_undo(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	int err = 0;

	struct buffer_action_multi_impl *impl =
		(struct buffer_action_multi_impl *) buffer_action_get_impl(action);

	struct list_node *node;
	list_for_each_reverse(list_tail(impl->action_list)->prev, node) {
		struct buffer_action_entry *entry =
			list_entry(node, struct buffer_action_entry, ln);

		buffer_action_t *a = entry->action;

		err = buffer_action_undo(a);
		if (err)
			break;
	}

	if (err) {
		struct list_node *start = node->next;
		list_for_each(start, node) {
			struct buffer_action_entry *entry =
				list_entry(node, struct buffer_action_entry, ln);

			buffer_action_t *a = entry->action;

			buffer_action_do(a);
		}
		return_error(err);
	}

	return 0;
}

static int buffer_action_multi_private_copy(buffer_action_t *action,
		data_object_t *cmp_dobj)
{
	if (action == NULL || cmp_dobj == NULL)
		return_error(EINVAL);

	struct buffer_action_multi_impl *impl =
		(struct buffer_action_multi_impl *) buffer_action_get_impl(action);

	struct list_node *node;
	list_for_each_reverse(list_tail(impl->action_list)->prev, node) {
		struct buffer_action_entry *entry =
			list_entry(node, struct buffer_action_entry, ln);

		buffer_action_t *a = entry->action;

		int err = buffer_action_private_copy(a, cmp_dobj);
		if (err)
			return_error(err);
	}

	return 0;
}

static int buffer_action_multi_to_event(buffer_action_t *action,
		struct bless_buffer_event_info *event_info)
{
	if (action == NULL || event_info == NULL)
		return_error(EINVAL);

	event_info->action_type = BLESS_BUFFER_ACTION_MULTI;
	event_info->range_start = -1;
	event_info->range_length = -1;
	event_info->save_fd = -1;

	return 0;
}

static int buffer_action_multi_free(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	struct buffer_action_multi_impl *impl =
		(struct buffer_action_multi_impl *) buffer_action_get_impl(action);

	/* Free contained actions */
	struct list_node *node;
	struct list_node *tmp;
	list_for_each_safe(list_head(impl->action_list)->next, node, tmp) {
		struct buffer_action_entry *entry =
			list_entry(node, struct buffer_action_entry, ln);

		buffer_action_t *a = entry->action;

		buffer_action_free(a);
		free(entry);
	}

	list_free(impl->action_list);
	free(impl);

	return 0;
}

