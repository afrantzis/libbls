/**
 * @file debug.h
 *
 */
#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#if ENABLE_DEBUG == 1

#include <stdio.h>
#include <string.h>

#define return_error(RET) do { \
	fprintf(stderr, "%s(%d): %s: %s\n", __FILE__, __LINE__, __func__, strerror(RET)); \
	return (RET); } while(0)


#else

#define return_error(RET) return (RET)

#endif


#ifdef __cplusplus
}
#endif

#endif /* _DEBUG_H */

