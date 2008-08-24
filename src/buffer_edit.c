/**
 * @file buffer_edit.c
 *
 * Buffer edit operations
 *
 * @author Michael Iatrou
 * @author Alexandros Frantzis
 */

#include "buffer.h"
#include "buffer_internal.h"

/**
 * Inserts data into a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to insert data into
 * @param offset the offset in the bless_buffer_t to insert data into
 * @param data a pointer to the data to insert
 * @param len the length of the data to insert
 *
 * @return the operation error code
 */
int bless_buffer_insert(bless_buffer_t *buf, off_t offset,
		void *data, size_t len)
{
	return -1;
}

/**
 * Delete data from a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to delete data from
 * @param offset the offset in the bless_buffer_t to delete data from
 * @param len the length of the data to delete
 *
 * @return the operation error code
 */
int bless_buffer_delete(bless_buffer_t *buf, off_t offset, size_t len)
{
	return -1;
}

/**
 * Reads data from a bless_buffer_t.
 *
 * @param src the bless_buffer_t to read data from
 * @param src_offset the offset in the bless_buffer_t to start reading from
 * @param dst a pointer to the start of the memory to store the data into
 * @param dst_offset the offset in dst to the start storing data into
 * @param len the length of the data to read
 *
 * @return the operation error code
 */
int bless_buffer_read(bless_buffer_t *src, off_t src_offset, void *dst,
		off_t dst_offset, size_t len)
{
	return -1;
}

/**
 * Copies data from a bless_buffer_t to another.
 *
 * @param src the bless_buffer_t to read data from
 * @param src_offset the offset in the source bless_buffer_t to start reading from
 * @param dst the bless_buffer_t to copy data to
 * @param dst_offset the offset in the destination bless_buffer_t to start copying data to
 * @param len the length of the data to read
 *
 * @return the operation error code
 */
int bless_buffer_copy(bless_buffer_t *src, off_t src_offset, bless_buffer_t *dst,
		off_t dst_offset, size_t len)
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
 * @param len the length of the data to search for
 * @param cb the bless_progress_cb to call to report the progress of the
 *           operation or NULL to disable reporting
 *
 * @return the offset in the bless_buffer_t where a match was found or
 *         an operation error code
 */
int bless_buffer_find(bless_buffer_t *buf, off_t *match, off_t start_offset, 
		void *data, size_t len, bless_progress_cb cb)
{
	return -1;
}
