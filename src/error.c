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
 * Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

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

