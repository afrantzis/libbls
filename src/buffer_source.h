/**
 * @file buffer_source.h
 *
 * Buffer source API
 */
#include <sys/types.h>

/**
 * Opaque type for a buffer source object.
 */
typedef void bless_buffer_source_t;

/**
 * A function to call to free the data related to a buffer source object.
 */
typedef int (bless_data_free_func)(void *);

int bless_buffer_source_memory(bless_buffer_source_t **src, void *data,
		size_t length, bless_data_free_func *data_free);

int bless_buffer_source_unref(bless_buffer_source_t *src);
