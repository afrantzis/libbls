/*
 * Copyright 2008, 2009 Alexandros Frantzis, Michael Iatrou
 *
 * This file is part of libbls.
 *
 * libbls is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * libbls is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * libbls.  If not, see <http://www.gnu.org/licenses/>.
 */

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

int data_object_tempfile_new(data_object_t **obj, int fd, char *path);

int data_object_file_set_close_func(data_object_t *obj,
        data_object_file_close_func *file_close);

/** @} */
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _DATA_OBJECT_FILE_H */

