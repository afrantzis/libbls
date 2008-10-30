/**
 * @file data_object.h
 *
 * Data object ADT API.
 *
 * @author Alexandros Frantzis
 */
#ifndef _DATA_OBJECT_H
#define _DATA_OBJECT_H

#include <sys/types.h>

/**
 * Opaque type for data object ADT.
 */
typedef struct data_object data_object_t;

int data_object_read(data_object_t *obj, void **buf, off_t offset, size_t len);

int data_object_write(data_object_t *obj, off_t offset, void *data, size_t len);

int data_object_free(data_object_t *obj);

int data_object_get_size(data_object_t *obj, size_t *size);

#endif
