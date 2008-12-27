/**
 * @file buffer_edit.c
 *
 * Buffer edit operations
 *
 * @author Michael Iatrou
 * @author Alexandros Frantzis
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


/********************/
/* Helper functions */
/********************/

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
		return err;

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
	return err;
}

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
	data_object_t *dobj;
	segment_get_data(seg, (void **)&dobj);

	/* user_data is actually a pointed to pointer */
	unsigned char **dst = (unsigned char **)user_data;

	int err = read_data_object(dobj, read_start, *dst, read_length);
	if (err)
		return err;

	/* Move the pointer forwards */
	*dst += read_length;

	return 0;
}

/*****************/
/* API Functions */
/*****************/

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
		return EINVAL;

	/* 
	 * No need to check for overflow, because it is detected by the
	 * functions that follow.
	 */

	segment_t *seg;

	int err = create_segment_from_source(&seg, src, src_offset, length);
	if (err)
		return err;
	
	segcol_t *sc = buf->segcol;

	/* Append segment to the segcol */
	err = segcol_append(sc, seg);
	if (err) 
		goto fail;

	return 0;
fail:
	/* No need to free obj, this is handled by segment_free */
	segment_free(seg);
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
		return EINVAL;

	/* 
	 * No need to check for overflow, because it is detected by the
	 * functions that follow.
	 */

	segment_t *seg;

	int err = create_segment_from_source(&seg, src, src_offset, length);
	if (err)
		return err;

	segcol_t *sc = buf->segcol;

	/* Insert segment to the segcol */
	err = segcol_insert(sc, offset, seg);
	if (err) 
		goto fail;

	return 0;
fail:
	/* No need to free obj, this is handled by segment_free */
	segment_free(seg);
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
		return EINVAL;

	/* 
	 * No need to check for overflow, valid ranges etc.
	 * They are all checked in segcol_delete().
	 */

	int err = segcol_delete(buf->segcol, NULL, offset, length);

	if (err)
		return err;

	return 0;
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
		return EINVAL;

	/* 
	 * Check for dst overflow (src_offset overflow is checked in
	 * segcol_foreach()).
	 */
	if (__MAX(size_t) - (size_t)dst < dst_offset)
		return EOVERFLOW;

	if (__MAX(size_t) - (size_t)dst - dst_offset < length - 1 * (length != 0))
		return EOVERFLOW;

	void *cur_dst = (unsigned char *)dst + dst_offset;

	int err = segcol_foreach(buf->segcol, src_offset, length,
			read_foreach_func, &cur_dst);
	if (err)
		return err;

	return 0;
}

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
	return -1;
}

/**
 * Searches for data in a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to search
 * @param[out] match the offset the data was found at or -1 if no match was found
 * @param start_offset the offset in the bless_buffer_t to start searching from
 * @param data a pointer to the data to search for
 * @param length the length of the data to search for
 * @param cb the bless_progress_cb to call to report the progress of the
 *           operation or NULL to disable reporting
 *
 * @return the offset in the bless_buffer_t where a match was found or
 *         an operation error code
 */
int bless_buffer_find(bless_buffer_t *buf, off_t *match, off_t start_offset, 
		void *data, size_t length, bless_progress_cb cb)
{
	return -1;
}
