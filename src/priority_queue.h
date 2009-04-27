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
 * @file priority_queue.h
 *
 * Priority queue API
 */

#ifndef _BLESS_PRIORITY_QUEUE_H
#define _BLESS_PRIORITY_QUEUE_H

#include <sys/types.h>

/**
 * @defgroup priority_queue Priority Queue
 *
 * A max-priority queue.
 *
 * @{
 */

/**
 * Opaque data type for a priority queue.
 */
typedef struct priority_queue priority_queue_t;

int priority_queue_new(priority_queue_t **pq, size_t size);

int priority_queue_free(priority_queue_t *pq);

int priority_queue_add(priority_queue_t *pq, void *data, int key, size_t *pos);

int priority_queue_remove_max(priority_queue_t *pq, void **data);

int priority_queue_change_key(priority_queue_t *pq, size_t pos, int key);

int priority_queue_get_size(priority_queue_t *pq, size_t *size);

/** @} */

#endif
