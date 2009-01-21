/**
 * @file error.h
 *
 * Libbless error
 */
#ifndef _BLESS_ERROR_H
#define _BLESS_ERROR_H

/* 
 * Although we don't need errno.h here ourselves, it makes life easier for 
 * users that include error.h through buffer.h.
 */
#include <errno.h>

/**
 * @defgroup bless_error Libbless error 
 *
 * @{
 */

/* Libbless specific errors */
#define BLESS_ENOTIMPL -1

const char *bless_strerror(int err);

/** @} */

#endif /* _BLESS_ERROR_H */

