/**
 * @file data_object_internal.h
 *
 * Definitions for internal use by the data_object_t implementations.
 */

#ifndef _DATA_OBJECT_INTERNAL_H
#define _DATA_OBJECT_INTERNAL_H

#include <sys/types.h>
#include "data_object.h"

/**
 * The functions in the data_object ADT that must be implemented.
 */
struct data_object_funcs {
	int (*get_data)(data_object_t *obj, void **buf, off_t offset,
		size_t *length, data_object_flags flags);
	int (*free)(data_object_t *obj);
	int (*get_size)(data_object_t *obj, off_t *size);
	int (*compare)(int *result, data_object_t *obj1, data_object_t *obj2);
};

int data_object_create_impl(data_object_t **obj, void *impl,
		struct data_object_funcs *funcs);

void *data_object_get_impl(data_object_t *obj);


#endif


