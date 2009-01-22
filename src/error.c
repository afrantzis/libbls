/**
 * @file error.c
 *
 * Library errors implementation
 */
#include <string.h>
#include "error.h"

static const char *error_desc[] = {
	"Not implemented",
};

#pragma GCC visibility push(default)

/**
 * Returns a string describing an error number.
 *
 * @param err the error number
 *
 * @returns a string describing the error number
 */
const char *bless_strerror(int err)
{
	if (err >= 0)
		return strerror(err);

	if (-err > sizeof(error_desc) / sizeof(*error_desc))
		return "Unknown error";
	else
		return error_desc[-err-1];
}

#pragma GCC visibility pop

