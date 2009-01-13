/**
 * @file data_object_memory.h
 *
 * Definitions of constructor functions for the memory data_object_t. 
 */
#ifndef _DATA_OBJECT_MEMORY_H
#define _DATA_OBJECT_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "data_object.h"
#include <sys/types.h>

/**
 * @addtogroup data_object
 * @{
 */

/**
 * A function used to free the data owned by a memory data_object_t.
 */
typedef void (data_object_memory_free_func)(void *);

/**
 * @name Constructors
 * @{
 */

int data_object_memory_new(data_object_t **obj, void *data, size_t size);

int data_object_memory_set_free_func(data_object_t *obj,
        data_object_memory_free_func *mem_free);

/** @} */
/** @} */

#ifdef __cplusplus
}
#endif

#endif

