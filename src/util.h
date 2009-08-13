/*
 * Copyright 2008,2009 Alexandros Frantzis, Michael Iatrou
 *
 * This file is part of libbls.
 *
 * libbls is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
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
 * @file util.h
 *
 * Utility functions and macros used by libbls.
 */
#ifndef _UTIL_H
#define _UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#define UNUSED_PARAM(x) (void)(x)

int path_join(char **result, char *path1, char *path2);

#ifdef __cplusplus
}
#endif

#endif /* _UTIL_H */

