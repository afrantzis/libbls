/**
 * @file data_object_file.h
 *
 * Definitions of constructor functions for the file data_object_t. 
 */
#ifndef _DATA_OBJECT_FILE_H
#define _DATA_OBJECT_FILE_H

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
 * A function used to close the file owned by a file data_object_t.
 */
typedef int (data_object_file_close_func)(int fd);

/**
 * @name Constructors
 * @{
 */

int data_object_file_new(data_object_t **obj, int fd);

int data_object_file_set_close_func(data_object_t *obj,
        data_object_file_close_func *file_close);

/** @} */
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _DATA_OBJECT_FILE_H */

