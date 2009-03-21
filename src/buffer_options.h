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
 * @file buffer_options.h
 *
 * Buffer options
 */
#ifndef _BLESS_BUFFER_OPTIONS_H
#define _BLESS_BUFFER_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

/** Buffer options */
typedef enum { 
	BLESS_BUF_TMP_DIR, /**< The directory to use for saving temporary files */
	BLESS_BUF_SENTINEL
} bless_buffer_option_t;

#ifdef __cplusplus
}
#endif

#endif /* _BLESS_BUFFER_OPTIONS_H */

