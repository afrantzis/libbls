/**
 * @file data_object_memory.h
 *
 * Definitions of constructor functions for the memory data_object_t. 
 */
#ifndef _DATA_OBJECT_MEMORY_H
#define _DATA_OBJECT_MEMORY_H

#include "data_object.h"
#include <sys/types.h>

int data_object_memory_new(data_object_t **obj, size_t size);

int data_object_memory_new_data(data_object_t **obj, void *data, size_t size);

#endif


