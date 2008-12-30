/**
 * @file buffer_internal.h
 *
 */
#ifndef _BLESS_BUFFER_INTERNAL_H
#define _BLESS_BUFFER_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "segcol.h"

/**
 * Bless buffer struct
 */
struct bless_buffer {
	segcol_t *segcol;

};

#ifdef __cplusplus
}
#endif

#endif /* _BLESS_BUFFER_INTERNAL_H */
