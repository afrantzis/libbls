/**
 * @file options.h
 *
 * Options
 */
#ifndef _OPTIONS_H
#define _OPTIONS_H

#include <sys/types.h>

/**
 * @defgroup options Options
 *
 *
 * @{
 */

/**
 * Opaque type for options ADT.
 */
typedef struct options options_t;

int options_new(options_t **opts, size_t nopts);

int options_get_option(options_t *opts, char **val, size_t key);

int options_set_option(options_t *opts, size_t key, char *val);

int options_free(options_t *opts);

/** @} */

#endif /* _OPTIONS_H */

