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
 * @file buffer_util.h
 *
 * Utility function used by bless_buffer_t
 */
#ifndef _BLESS_BUFFER_UTIL_H
#define _BLESS_BUFFER_UTIL_H 

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "buffer.h"
#include "segcol.h"
#include "segment.h"
#include "data_object.h"
#include "list.h"

typedef int (segcol_foreach_func)(segcol_t *segcol, segment_t *seg,
		off_t mapping, off_t read_start, off_t read_length, void *user_data);

int read_data_object(data_object_t *dobj, off_t offset, void *mem, off_t length);

int write_data_object(data_object_t *dobj, off_t offset, off_t length,
		int fd, off_t file_offset);

int segcol_foreach(segcol_t *segcol, off_t offset, off_t length,
		segcol_foreach_func *func, void *user_data);

int segcol_store_in_memory(segcol_t *segcol, off_t offset, off_t length);

int segcol_store_in_file(segcol_t *segcol, off_t offset, off_t length,
		char *tmpdir);

int segcol_add_copy(segcol_t *dst, off_t offset, segcol_t *src);

int undo_list_enforce_limit(bless_buffer_t *buf, int ensure_vacancy);

int action_list_clear(list_t *action_list);

#ifdef __cplusplus
}
#endif

#endif 
