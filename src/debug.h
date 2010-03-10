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

#define print_error(ERR) do {\
	fprintf(stderr, "%s(%d): %s: %s\n", __FILE__, __LINE__, __func__, strerror(ERR)); \
	} while(0)

#else

#define print_error(ERR) do { } while(0)

#endif

#define return_error(ERR) do { int _local_err = (ERR); print_error(_local_err); return _local_err; } while(0)
#define goto_error(ERR, LABEL) do { print_error(ERR); goto LABEL; } while(0)


#ifdef __cplusplus
}
#endif

#endif /* _DEBUG_H */

