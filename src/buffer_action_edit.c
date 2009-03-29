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
 * Foobar.  If not, see <http://www.gnu.org/licenses/>.
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
#include "util.h"


/* Forward declarations */

/* Helper functions */
static int create_segment_from_source(segment_t **seg, bless_buffer_source_t *src,
		off_t src_offset, off_t length);

/* API functions */
static int buffer_action_append_do(buffer_action_t *action);
static int buffer_action_append_undo(buffer_action_t *action);
static int buffer_action_append_free(buffer_action_t *action);
static int buffer_action_insert_do(buffer_action_t *action);
static int buffer_action_insert_undo(buffer_action_t *action);
static int buffer_action_insert_free(buffer_action_t *action);
static int buffer_action_delete_do(buffer_action_t *action);
static int buffer_action_delete_undo(buffer_action_t *action);
static int buffer_action_delete_free(buffer_action_t *action);

/* Action functions */
static struct buffer_action_funcs buffer_action_append_funcs = {
	.do_func = buffer_action_append_do,
	.undo_func = buffer_action_append_undo,
	.free_func = buffer_action_append_free
};

static struct buffer_action_funcs buffer_action_insert_funcs = {
	.do_func = buffer_action_insert_do,
	.undo_func = buffer_action_insert_undo,
	.free_func = buffer_action_insert_free
};

static struct buffer_action_funcs buffer_action_delete_funcs = {
	.do_func = buffer_action_delete_do,
	.undo_func = buffer_action_delete_undo,
	.free_func = buffer_action_delete_free
};

/* Action implementations */
struct buffer_action_append_impl {
	bless_buffer_t *buf;
	bless_buffer_source_t *src;
	off_t src_offset;
	off_t length;
};

struct buffer_action_insert_impl {
	bless_buffer_t *buf;
	off_t offset;
	bless_buffer_source_t *src;
	off_t src_offset;
	off_t length;
};

struct buffer_action_delete_impl {
	bless_buffer_t *buf;
	off_t offset;
	off_t length;
	segcol_t *deleted;
};

/********************
 * Helper functions *
 ********************/

/**
 * Create a segment from a bless_buffer_source_t.
 *
 * @param[out] seg the created segment
 * @param src the source
 * @param src_offset the start of the segment range in src
 * @param length the length of the segment range
 *
 * @return the operation error code
 */
static int create_segment_from_source(segment_t **seg, bless_buffer_source_t *src,
		off_t src_offset, off_t length)
{
	data_object_t *obj = (data_object_t *) src;

	/* Create a segment pointing to the data object */
	int err = segment_new(seg, obj, src_offset, length, data_object_update_usage);
	if (err)
		return_error(err);

	/* 
	 * Check if the specified file range is valid. This is done 
	 * here so that overflow has already been checked by segment_new().
	 */
	off_t obj_size;
	err = data_object_get_size(obj, &obj_size);
	if (err)
		goto fail;

	if (src_offset + length - 1 * (length != 0) >= obj_size) {
		err = EINVAL;
		goto fail;
	}

	return 0;

fail:
	/* No need to free obj, this is handled by segment_free */
	segment_free(*seg);
	return_error(err);
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
		goto fail;

	/* Update usage count of data object */
	data_object_t *obj = (data_object_t *) src;
	err = data_object_update_usage(obj, 1);
	if (err)
		goto fail;

	/* Initialize implementation */
	impl->buf = buf;
	impl->src = src;
	impl->src_offset = src_offset;
	impl->length = length;

	return 0;

fail:
	free(impl);
	return_error(err);
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
		goto fail;

	/* Update usage count of data object */
	data_object_t *obj = (data_object_t *) src;
	err = data_object_update_usage(obj, 1);
	if (err)
		goto fail;

	/* Initialize implementation */
	impl->buf = buf;
	impl->offset = offset;
	impl->src = src;
	impl->src_offset = src_offset;
	impl->length = length;

	return 0;

fail:
	free(impl);
	return_error(err);
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
		goto fail;

	/* Initialize implementation */
	impl->buf = buf;
	impl->offset = offset;
	impl->length = length;
	impl->deleted = NULL;

	return 0;

fail:
	free(impl);
	return_error(err);
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

	int err = create_segment_from_source(&seg, impl->src, impl->src_offset,
			impl->length);

	if (err)
		return_error(err);
	
	segcol_t *sc = impl->buf->segcol;

	/* Append segment to the segcol */
	err = segcol_append(sc, seg);
	if (err) 
		goto fail;

	return 0;

fail:
	segment_free(seg);
	return_error(err);
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

	/* Delete range from the segcol */
	err = segcol_delete(sc, NULL, segcol_size - impl->length, impl->length);
	if (err) 
		return_error(err);

	return 0;
}

static int buffer_action_append_free(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	struct buffer_action_append_impl *impl =
		(struct buffer_action_append_impl *) buffer_action_get_impl(action);

	data_object_t *obj = (data_object_t *) impl->src;
	int err = data_object_update_usage(obj, -1);
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

	int err = create_segment_from_source(&seg, impl->src, impl->src_offset,
			impl->length);

	if (err)
		return_error(err);
	
	segcol_t *sc = impl->buf->segcol;

	/* Append segment to the segcol */
	err = segcol_insert(sc, impl->offset, seg);
	if (err) 
		goto fail;

	return 0;
fail:
	segment_free(seg);
	return_error(err);
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

	/* Delete range from the segcol */
	int err = segcol_delete(sc, NULL, impl->offset, impl->length);
	if (err) 
		return_error(err);

	return 0;
}

static int buffer_action_insert_free(buffer_action_t *action)
{
	if (action == NULL)
		return_error(EINVAL);

	struct buffer_action_insert_impl *impl =
		(struct buffer_action_insert_impl *) buffer_action_get_impl(action);

	data_object_t *obj = (data_object_t *) impl->src;
	int err = data_object_update_usage(obj, -1);
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

	int err = segcol_add_copy(impl->buf->segcol, impl->offset, impl->deleted);
	if (err)
		return_error(err);
		
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
			return err;
	}

	free(impl);

	return 0;
}

