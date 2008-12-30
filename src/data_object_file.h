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
 * @name Constructors
 * @{
 */

int data_object_file_new(data_object_t **obj, int fd);

/** @} */
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _DATA_OBJECT_FILE_H */

