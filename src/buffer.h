/**
 * @file buffer.h
 *
 * Public libbless buffer API.
 *
 * @author Alexandros Frantzis
 * @author Michael Iatrou
 */
#ifndef _BLESS_BUFFER_H
#define _BLESS_BUFFER_H

#include <unistd.h>

/**
 * @defgroup buffer Buffer Module
 *
 * @{
 */

/**
 * Opaque data type for a bless buffer.
 */
typedef struct bless_buffer bless_buffer_t;

/** 
 * Callback called to report the progress of long operations.
 *
 * This callback is used by operations that may take a long time to finish.
 * These operations call the callback periodically and pass progress info using
 * the info argument. The return value of the callback function is checked by
 * the operations to decide whether they should continue.
 *
 * @param info operation specific progress info
 *
 * @return 1 if the operation must be cancelled, 0 otherwise
 */
typedef int (*bless_progress_cb)(void *info);

/**
 * @name File Operations
 *
 * @{
 */

int bless_buffer_new(bless_buffer_t **buf);

int bless_buffer_new_from_file(bless_buffer_t **buf, int fd);

int bless_buffer_save(bless_buffer_t *buf, int fd, bless_progress_cb cb);

int bless_buffer_free(bless_buffer_t *buf);

/** @} */
/**
 * @name Edit Operations
 *
 * @{
 */

int bless_buffer_append(bless_buffer_t *buf, void *data, size_t length);

int bless_buffer_insert(bless_buffer_t *buf, off_t offset,
		void *data, size_t length);

int bless_buffer_delete(bless_buffer_t *buf, off_t offset, off_t length);

int bless_buffer_read(bless_buffer_t *src, off_t src_offset, void *dst,
		size_t dst_offset, size_t length);

int bless_buffer_copy(bless_buffer_t *src, off_t src_offset, bless_buffer_t *dst,
		off_t dst_offset, off_t length);

int bless_buffer_find(bless_buffer_t *buf, off_t *match, off_t start_offset, 
		void *data, size_t length, bless_progress_cb cb);

/** @} */
/**
 * @name Undo - Redo Operations
 *
 * @{
 */

int bless_buffer_undo(bless_buffer_t *buf);

int bless_buffer_redo(bless_buffer_t *buf);

int bless_buffer_begin_multi_op(bless_buffer_t *buf);

int bless_buffer_end_multi_op(bless_buffer_t *buf);


/** @} */
/**
 * @name Buffer Information
 *
 * @{
 */

int bless_buffer_can_undo(bless_buffer_t *buf, int *can_undo);

int bless_buffer_can_redo(bless_buffer_t *buf, int *can_redo);

int bless_buffer_get_fd(bless_buffer_t *buf, int *fd);

int bless_buffer_get_size(bless_buffer_t *buf, off_t *size);

/** @} */

/** @} */

#endif /* _BLESS_BUFFER_H */
