/**
 * @file buffer_undo.c
 *
 * Buffer undo operations
 *
 * @author Alexandros Frantzis
 * @author Michael Iatrou
 */

#include "buffer.h"
#include "buffer_internal.h"

/**
 * Undoes the last operation in a bless_buffer_t.
 *
 * @param buf the bless_buffer_t to undo the operation in
 *
 * @return the operation error code
 */
int bless_buffer_undo(bless_buffer_t *buf)
{
	return -1;
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
	return -1;
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
	return -1;
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
	return -1;
}

