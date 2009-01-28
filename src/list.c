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
 * @file list.c
 *
 * List implementation
 */
#include "list.h"
#include <errno.h>
#include <stdlib.h>
#include "util.h"

/**
 * Creates a new doubly linked list.
 *
 * @param[out] list the created list
 * @param entry_size the size of each list entry
 * @param ln_offset the offset of the struct list_node in the list entries
 *
 * @return the operation error code
 */
int _list_new(struct list **list, size_t entry_size, size_t ln_offset)
{
	if (list == NULL)
		return_error(EINVAL);

	void *head;
	void *tail;

	int err = _list_new_entry(&head, entry_size, ln_offset);
	if (err)
		return_error(err);

	err = _list_new_entry(&tail, entry_size, ln_offset);
	if (err) {
		free(head);
		return_error(err);
	}

	*list = malloc(sizeof **list);
	if (*list == NULL) {
		free(head);
		free(tail);
		return 0;
	}
	
	/* Calculate pointers to list nodes */
	struct list_node *head_ln =
		(struct list_node *)((char *)head + ln_offset);

	struct list_node *tail_ln =
		(struct list_node *)((char *)tail + ln_offset);

	/* Connect head and tail nodes */
	head_ln->next = tail_ln;
	head_ln->prev = head_ln;

	tail_ln->next = tail_ln;
	tail_ln->prev = head_ln;

	(*list)->head = head;
	(*list)->tail = tail;

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
int _list_free(struct list *list, size_t ln_offset)
{
	if (list == NULL)
		return_error(EINVAL);

	struct list_node *node;
	struct list_node *tmp;

	list_for_each_safe(_list_node(list->head, ln_offset)->next, node, tmp) {
		void *entry = _list_entry(node, ln_offset);
		free(entry);
	}

	free(list->head);
	free(list->tail);
	free(list);

	return 0;
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
	if (p == NULL || q == NULL)
		return_error(EINVAL);

	q->next = p->next;
	q->prev = p;

	p->next->prev = q;

	p->next = q;

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

/**
 * Creates a new list entry.
 *
 * @param[out] entry the new list entry
 * @param entry_size the size of each list entry
 * @param ln_offset the offset of the struct list_node in the list entries
 *
 * @return the operation error code
 */
int _list_new_entry(void **entry, size_t entry_size, size_t ln_offset)
{
	if (entry == NULL)
		return_error(EINVAL);

	*entry = calloc(1, entry_size);

	if (*entry == NULL)
		return_error(ENOMEM);

	struct list_node *ln =
		(struct list_node *)((unsigned char *)*entry + ln_offset);

	ln->prev = NULL;
	ln->next = NULL;

	return 0;
}

