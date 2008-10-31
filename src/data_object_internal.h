/**
 * @file data_object_internal.h
 *
 * Definitions for internal use by the data_object_t implementations.
 */

#ifndef _DATA_OBJECT_INTERNAL_H
#define _DATA_OBJECT_INTERNAL_H

/**
 * The functions in the data_object ADT that must be implemented.
 */
struct data_object_funcs {
	int (*read)(data_object_t *obj, void **buf, off_t start, size_t len);
	int (*write)(data_object_t *obj, off_t offset, void *data, size_t len);
	int (*free)(data_object_t *obj);
	int (*get_size)(data_object_t *obj, off_t *size);
};

int data_object_create_impl(data_object_t **obj, void *impl,
		struct data_object_funcs *funcs);

void *data_object_get_impl(data_object_t *obj);


#endif


