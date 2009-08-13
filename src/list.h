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
 * @file list.h
 *
 * List API
 */
#ifndef _BLESS_LIST_H
#define _BLESS_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/** 
 * Iterate safely through the nodes in a list.
 *
 * This macro should be used when nodes are going to be altered or
 * deleted during the iteration.
 * 
 * @param first the list node to start from
 * @param node a struct list_node pointer that will hold the current node in
 *             each iteration
 * @param tmp a struct list_node pointer that will be used internally for safe
 *            iteration
 */
#define list_for_each_safe(first, node, tmp) \
	for ((node) = (first), (tmp) = (node)->next; (node) != (node)->next; \
			(node) = (tmp), (tmp) = (tmp)->next)

/** 
 * Reverse iterate safely through the nodes in a list.
 *
 * This macro should be used when nodes are going to be altered or
 * deleted during the iteration.
 * 
 * @param last the list node to start from
 * @param node a struct list_node pointer that will hold the current node in
 *             each iteration
 * @param tmp a struct list_node pointer that will be used internally for safe
 *            iteration
 */
#define list_for_each_reverse_safe(last, node, tmp) \
	for ((node) = (last), (tmp) = (node)->prev; (node) != (node)->prev; \
			(node) = (tmp), (tmp) = (tmp)->prev)

/** 
 * Iterate through the nodes in a list.
 *
 * If nodes are going to be altered or deleted during the iteration use
 * list_for_each_safe().
 * 
 * @param first the list node to start from
 * @param node a struct list_node pointer that will hold the current node in
 *             each iteration
 */
#define list_for_each(first, node) \
	for ((node) = (first); (node) != (node)->next; (node) = (node)->next)

/** 
 * Reverse iterate through the nodes in a list.
 *
 * If nodes are going to be altered or deleted during the iteration use
 * list_for_each_reverse_safe().
 * 
 * @param last the list node to start from
 * @param node a struct list_node pointer that will hold the current node in
 *             each iteration
 */
#define list_for_each_reverse(last, node) \
	for ((node) = (last); (node) != (node)->prev; (node) = (node)->prev)

/**
 * Gets the entry containing a list node.
 *
 * @param ptr pointer to a struct list_node
 * @param type the type of the entry containing the list node
 * @param member the member of the type storing the list node
 *
 * @return a (type *) pointer to the entry
 */
#define list_entry(ptr, type, member) \
	((type *)_list_entry((ptr), (size_t)&((type *)0)->member))

/**
 * Gets the entry containing a list node.
 *
 * @param ptr pointer to a struct list_node
 * @param ln_offset the offset in bytes in the entry the list node is stored at
 *
 * @return a (type *) pointer to the entry
 */
#define _list_entry(ptr, ln_offset) \
	(void *)((char *)(ptr)-(ln_offset))

/**
 * Creates a new list.
 *
 * @param[out] pptr pointer to the created list
 * @param type the type of the entry containing the list nodes
 * @param member the member of the type storing the list nodes
 *
 * @return the operation error code
 */
#define list_new(pptr, type, member) \
	_list_new((pptr), (size_t)&((type *)0)->member)


typedef struct list list_t;

struct list_node {
	struct list_node *next;
	struct list_node *prev;
};

int _list_new(list_t **list, size_t ln_offset);
int list_free(list_t *list);

struct list_node *list_head(list_t *list);
struct list_node *list_tail(list_t *list);

int list_insert_before(struct list_node *p, struct list_node *q);
int list_insert_after(struct list_node *p, struct list_node *q);
int list_insert_chain_after(struct list_node *p, struct list_node *start,
		struct list_node *end);
int list_delete_chain(struct list_node *start, struct list_node *end);

#ifdef __cplusplus
}
#endif

#endif
