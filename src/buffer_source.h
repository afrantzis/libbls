/**
 * @file buffer_source.h
 *
 * Buffer source API
 */
#ifndef _BLESS_BUFFER_SOURCE_H
#define _BLESS_BUFFER_SOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/**
 * @addtogroup buffer
 *
 * @{
 */

/**
 * @name Buffer Sources
 */

/**
 * Opaque type for a buffer source object.
 */
typedef void bless_buffer_source_t;

/**
 * A function to call to free the memory associated with a memory source object.
 */
typedef void (bless_mem_free_func)(void *);

/**
 * A function to call to close the file associated with a file source object.
 */
typedef int (bless_file_close_func)(int);

int bless_buffer_source_memory(bless_buffer_source_t **src, void *data,
		size_t length, bless_mem_free_func *mem_free);

int bless_buffer_source_file(bless_buffer_source_t **src, int fd,
		bless_file_close_func *file_close);

int bless_buffer_source_unref(bless_buffer_source_t *src);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif
