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
 * @file buffer_internal.h
 *
 */
#ifndef _BLESS_BUFFER_INTERNAL_H
#define _BLESS_BUFFER_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "segcol.h"
#include "options.h"
#include "list.h"
#include "buffer_action.h"

/* Helper macros for action list */
#define action_list_head(ptr) list_head((ptr), struct buffer_action_entry, ln)
#define action_list_tail(ptr) list_tail((ptr), struct buffer_action_entry, ln)

/** 
 * Buffer action list entry.
 */
struct buffer_action_entry {
	struct list_node ln;
	buffer_action_t *action;
};

/**
 * Bless buffer struct
 */
struct bless_buffer {
	segcol_t *segcol;
	options_t *options;
	struct list *undo_list;
	struct list *redo_list;
};

#ifdef __cplusplus
}
#endif

#endif /* _BLESS_BUFFER_INTERNAL_H */
