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
 * libbls.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file segcol_list.h
 *
 * Definition of constructor function for the list implementation of segcol_t.
 */
#ifndef _SEGCOL_LIST_H
#define _SEGCOL_LIST_H

#include "segcol.h"

/**
 * @addtogroup segcol
 * @{
 */

/**
 * @name Constructors
 * @{
 */

int segcol_list_new(segcol_t **segcol);

/** @} */
/** @} */

#endif /* _SEGCOL_LIST_H */

