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
 * @file error.h
 *
 * Library errors
 */
#ifndef _BLESS_ERROR_H
#define _BLESS_ERROR_H

/* 
 * Although we don't need errno.h here ourselves, it makes life easier for 
 * users that include error.h through buffer.h.
 */
#include <errno.h>

/**
 * @defgroup bless_error Library error 
 *
 * @{
 */

/* Library specific errors */
#define BLESS_ENOTIMPL -1

const char *bless_strerror(int err);

/** @} */

#endif /* _BLESS_ERROR_H */

