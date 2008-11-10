/**
 * @file data_object_file.h
 *
 * Definitions of constructor functions for the file data_object_t. 
 */
#ifndef _DATA_OBJECT_FILE_H
#define _DATA_OBJECT_FILE_H

#include "data_object.h"
#include <sys/types.h>

int data_object_file_new(data_object_t **obj, int fd);

#endif /* _DATA_OBJECT_FILE_H */



