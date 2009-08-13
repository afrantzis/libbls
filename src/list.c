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
 * @file list.c
 *
 * List implementation
 */
#include "list.h"
#include <errno.h>
#include <stdlib.h>
#include "debug.h"

/**
 * A doubly linked list
 */
struct list {
	size_t ln_offset; /**< The offset of a node in an entry for this list */
	struct list_node head; /**< The head node of the list */
	struct list_node tail; /**< The tail node of the list */
};

/**
 * Creates a new doubly linked list.
 *
 * @param[out] list the created list
 * @param ln_offset the offset of the struct list_node in the list entries
 *
 * @return the operation error code
 */
int _list_new(list_t **list, size_t ln_offset)
{
	if (list == NULL)
		return_error(EINVAL);

	list_t *l = malloc(sizeof **list);
	if (l == NULL)
		return_error(ENOMEM);

	l->ln_offset = ln_offset;

	/* Connect head and tail nodes */
	l->head.next = &l->tail;
	l->head.prev = &l->head;

	l->tail.next = &l->tail;
	l->tail.prev = &l->head;;

	*list = l;

	return 0;
}

/** 
 * Frees a list.
 *
 * This function does not free the data stored in the list.
 * 
 * @param list the list to free 
 * @param ln_offset the offset of the struct list_node in the list entries
 * 
 * @return the operation error code 
 */
int list_free(list_t *list)
{
	if (list == NULL)
		return_error(EINVAL);

	struct list_node *node;
	struct list_node *tmp;

	list_for_each_safe(list->head.next, node, tmp) {
		void *entry = _list_entry(node, list->ln_offset);
		free(entry);
	}

	free(list);

	return 0;
}

/**
 * Gets the head node of a list
 *
 * @param list pointer to the list
 *
 * @return a list_node pointer to the head node of the list
 */
struct list_node *list_head(list_t *list)
{
	return &list->head;
}

/**
 * Gets the tail node of a list
 *
 * @param list pointer to the list
 *
 * @return a list_node pointer to the tail node of the list
 */
struct list_node *list_tail(list_t *list)
{
	return &list->tail;
}

/**
 * Inserts a node before another node in a list.
 *
 * @param p the node to which the new noded is inserted before
 * @param q the node to insert
 *
 * @return the operation error code
 */
int list_insert_before(struct list_node *p, struct list_node *q)
{
	if (p == NULL || q == NULL)
		return_error(EINVAL);

	q->next = p;
	q->prev = p->prev;

	p->prev->next = q;

	p->prev = q;

	return 0;
}

/**
 * Inserts a node after another node in a list.
 *
 * @param p the node to which the new noded is inserted after
 * @param q the node to insert
 *
 * @return the operation error code
 */
int list_insert_after(struct list_node *p, struct list_node *q)
{
	return list_insert_chain_after(p, q, q);
}


/**
 * Inserts a chain of nodes after another node in a list.
 *
 * @param p the node after which the node chain is inserted
 * @param first the first node in the chain to insert
 * @param last the last node in the chain to insert
 *
 * @return the operation error code
 */
int list_insert_chain_after(struct list_node *p, struct list_node *first,
		struct list_node *last)
{
	if (p == NULL || first == NULL || last == NULL)
		return_error(EINVAL);

	last->next = p->next;
	first->prev = p;

	last->next->prev = last;

	p->next = first;

	return 0;
}

/**
 * Deletes a chain of nodes from the list.
 *
 * This operation doesn't free the memory
 * occupied by the nodes.
 *
 * @param first the first node in the chain to delete
 * @param last the last node in the chain to delete
 *
 * @return the operation error code
 */
int list_delete_chain(struct list_node *first, struct list_node *last)
{
	if (first == NULL || last == NULL)
		return_error(EINVAL);

	struct list_node *prev_node = first->prev;
	struct list_node *next_node = last->next;

	next_node->prev = prev_node;

	prev_node->next = next_node;

	first->prev = NULL;
	last->next = NULL;

	return 0;
}

