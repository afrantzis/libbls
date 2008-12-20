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
