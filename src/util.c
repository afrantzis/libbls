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
 * @file util.c
 *
 * Implementation of utility functions used by libbls.
 */

#include "util.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include "sys/types.h"

/** 
 * Joins two path components to form a full path.
 *
 * The resulting path should be freed using free() when it is not needed
 * anymore.
 * 
 * @param[out] result the resulting combined path
 * @param path1 first path component
 * @param path2 second path component
 * 
 * @return the operation error code
 */
int path_join(char **result, char *path1, char *path2)
{
	if (result == NULL || path1 == NULL || path2 == NULL)
		return_error(EINVAL);

	/* TODO: Path separator depends on platform! */
	char *sep = "/";
	int need_sep = 0;

	size_t result_size = strlen(path1) + strlen(path2) + 1;

	/* Check if we need to add a path separator between the two components. */
	if (path1[strlen(path1) - 1] != sep[0]) {
		need_sep = 1;
		result_size += 1;
	}

	*result = malloc(result_size);
	if (*result == NULL)
		return_error(ENOMEM);

	strncpy(*result, path1, result_size);

	if (need_sep == 1)
		strncat(*result, sep, result_size - strlen(*result) - 1);

	strncat(*result, path2, result_size - strlen(*result) - 1);

	return 0;
}

