/**
 * @file buffer_util.h
 *
 * Utility function used by bless_buffer_t
 */
#ifndef _BLESS_BUFFER_UTIL_H
#define _BLESS_BUFFER_UTIL_H 

#include <sys/types.h>
#include "segcol.h"
#include "segment.h"
#include "data_object.h"

typedef int (segcol_foreach_func)(segcol_t *segcol, segment_t *seg,
		off_t mapping, off_t read_start, off_t read_length, void *user_data);

int read_data_object(data_object_t *dobj, off_t offset, void *mem, off_t length);

int segcol_foreach(segcol_t *segcol, off_t offset, off_t length,
		segcol_foreach_func *func, void *user_data);

#endif 
