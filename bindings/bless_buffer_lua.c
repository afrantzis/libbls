/*
 * Copyright 2009 Alexandros Frantzis
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
 * @file bless_buffer_lua.c
 *
 * Lua bindings for the libbls library.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "buffer.h"

#pragma GCC visibility push(default)

static int optboolean(lua_State *L, int narg, int def)
{
	if (lua_type(L, narg) <= 0)
		return def;
	
	if (!lua_isboolean(L, narg))
		luaL_typerror(L, narg, "boolean");
	
	return lua_toboolean(L, narg);
}

static int should_throw_errors(lua_State *L)
{
	int throw_errors;
	
	lua_getglobal(L, "bless");
	lua_getfield(L, -1, "throw_errors");
	
	throw_errors = optboolean(L, -1, 1);
	lua_pop(L, 2);
	
	return throw_errors;
}

static int push_global_bless(lua_State *L)
{
	lua_getglobal(L, "bless");
	if (lua_type(L, -1) <= 0) {
		lua_pop(L, 1);
		return 0;
	}
	
	return 1;
}

/*
 * Bytearray bindings
 */
struct bytearray {
	int free_on_finalize;
	size_t length;
	uint8_t *data;
};

static struct bytearray *push_bytearray(lua_State *L)
{
	struct bytearray *ba =
		(struct bytearray *)lua_newuserdata(L, sizeof(*ba));

	luaL_getmetatable(L, "bless.bytearray");
	lua_setmetatable(L, -2);
	
	return ba;
}

static struct bytearray *to_bytearray(lua_State *L, int index)
{
	struct bytearray *ba =
		(struct bytearray *) luaL_checkudata(L, index, "bless.bytearray");
	
	if (ba == NULL)
		luaL_typerror(L, index, "bless.bytearray");
	
	return ba;
}

static int bytearray_new(lua_State *L)
{
	size_t length = (size_t)luaL_checknumber(L, 1);
	int use_gc = optboolean(L, 2, 1);
	
	struct bytearray *ba = push_bytearray(L);
	
	ba->data = malloc(length);
	ba->length = length;
	ba->free_on_finalize = use_gc;

	return 1;
}

static uint8_t *bytearray_getelem(lua_State *L)
{
	struct bytearray *ba = to_bytearray(L, 1);
	size_t index = (size_t)luaL_checknumber(L, 2);

	luaL_argcheck(L, 1 <= index && index <= ba->length, 2,
			"index out of range");

	/* return element address */
	return &ba->data[index - 1];
} 

static int bytearray_gc(lua_State *L)
{
	struct bytearray *ba = to_bytearray(L, 1);
	if (ba->free_on_finalize)
		free(ba->data);
	
	return 0;
}

static int bytearray_set(lua_State *L)
{
	double val = luaL_checknumber(L, 3);
	luaL_argcheck(L, 0 <= val && val <= 255, 3,
			"val out of range");
	*bytearray_getelem(L) = (uint8_t) val;
	
	return 0;
}

static int bytearray_get(lua_State *L)
{
	lua_pushnumber(L, *bytearray_getelem(L));
	return 1;
}

static int bytearray_length(lua_State *L)
{
	struct bytearray *ba = to_bytearray(L, 1);
	lua_pushnumber(L, ba->length);
	
	return 1;
}

static const luaL_reg bytearray_methods[] = {
	{"new"    , bytearray_new},
	{0,0}
};

static const luaL_reg bytearray_meta[] = {
	{"__gc", bytearray_gc},
	{"__index", bytearray_get},
	{"__newindex", bytearray_set},
	{"__len", bytearray_length},
	{0, 0}
};

/*
 * Buffer source bindings
 */
static bless_buffer_source_t **push_buffer_source(lua_State *L,
		bless_buffer_source_t *source)
{
	bless_buffer_source_t **s =
		(bless_buffer_source_t **)lua_newuserdata(L, sizeof(*s));

	*s = source;

	luaL_getmetatable(L, "bless.buffer_source");
	lua_setmetatable(L, -2);
	return s;
}

static bless_buffer_source_t *to_buffer_source(lua_State *L, int index)
{
	bless_buffer_source_t **s =
		(bless_buffer_source_t **) luaL_checkudata(L, index, "bless.buffer_source");
	
	if (s == NULL)
		luaL_typerror(L, index, "bless.buffer_source");
	
	return *s;
}

static int buffer_source_lua_memory(lua_State *L)
{
	bless_buffer_source_t *src = NULL;
	size_t length = 0;
	uint8_t *data = NULL;
	struct bytearray *ba = NULL;
	
	if (lua_isstring(L, 1)) {
		const char *string = lua_tolstring (L, 1, &length);
		if (string == NULL)
			luaL_typerror(L, 1, "string");

		data = malloc(length);
		if (data == NULL)
			luaL_error(L, "memory");

		memcpy(data, string, length);
	}
	else {
		ba = luaL_checkudata(L, 1, "bless.bytearray");
		data = ba->data;
		length = ba->length;
	}
	
	int err = bless_buffer_source_memory(&src, data, length, free);
	if (err && should_throw_errors(L))
		return luaL_error(L, "Error creating buffer from memory: %s", strerror(err));
	
	lua_pushnumber(L, err);
	push_buffer_source(L, src);

	return 2;
}

static int buffer_source_lua_file(lua_State *L)
{
	bless_buffer_source_t *src = NULL;
	FILE *fp = *(FILE **)luaL_checkudata(L, 2, LUA_FILEHANDLE);
	int fd = fileno(fp);
	
	fd = dup(fd);
	
	int err = bless_buffer_source_file(&src, fd, close);
	if (err && should_throw_errors(L))
		return luaL_error(L, "%s", strerror(err));
	
	lua_pushnumber(L, err);
	push_buffer_source(L, src);

	return 2;
}

static int buffer_source_lua_gc(lua_State *L)
{
	bless_buffer_source_t *src = to_buffer_source(L, 1);
	if (src != NULL)
		bless_buffer_source_unref(src);
	
	return 0;
}

static const luaL_reg buffer_source_methods[] = {
	{"memory", buffer_source_lua_memory},
	{"file", buffer_source_lua_file},
	{0,0}
};

static const luaL_reg buffer_source_meta[] = {
	{"__gc", buffer_source_lua_gc},
	{0, 0}
};

/*
 * Bless buffer bindings
 */
static bless_buffer_t **push_buffer(lua_State *L,
		bless_buffer_t *buf)
{
	bless_buffer_t **b =
		(bless_buffer_t **)lua_newuserdata(L, sizeof(*b));

	*b = buf;

	luaL_getmetatable(L, "bless.buffer");
	lua_setmetatable(L, -2);
	
	return b;
}

static bless_buffer_t *to_buffer(lua_State *L, int index)
{
	bless_buffer_t **b =
		(bless_buffer_t **) luaL_checkudata(L, index, "bless.buffer");
	
	if (b == NULL)
		luaL_typerror(L, index, "bless.buffer");
	
	return *b;
}

static int buffer_lua_new(lua_State *L)
{
	bless_buffer_t *buf = NULL;
	
	int err = bless_buffer_new(&buf);
	if (err && should_throw_errors(L))
		return luaL_error(L, "Error creating new buffer: %s", strerror(err));
	
	lua_pushnumber(L, err);
	push_buffer(L, buf);
	
	/* Associate an environment table containing a reference count */
	lua_newtable(L);
	lua_pushliteral(L, "refcount");
	lua_pushinteger(L, 0);
	lua_rawset(L, -3);
	
	lua_setfenv(L, -2);

	return 2;
}

static int buffer_lua_save(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	FILE *fp = *(FILE **)luaL_checkudata(L, 2, LUA_FILEHANDLE);
	int fd = fileno(fp);
	
	int err = bless_buffer_save(buf, fd, NULL);
	if (err && should_throw_errors(L))
		return luaL_error(L, "Error saving buffer: %s", strerror(err));
	
	lua_pushnumber(L, err);

	return 1;
}

static int buffer_lua_append(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	bless_buffer_source_t *src = to_buffer_source(L, 2);
	off_t offset = (off_t)luaL_checknumber(L, 3);
	off_t length = (off_t)luaL_checknumber(L, 4);
	
	int err = bless_buffer_append(buf, src, offset, length);
	if (err && should_throw_errors(L))
		return luaL_error(L, "Error appending data to buffer: %s", strerror(err));
	
	lua_pushnumber(L, err);

	return 1;
}

static int buffer_lua_insert(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	off_t offset = (off_t)luaL_checknumber(L, 2);
	bless_buffer_source_t *src = to_buffer_source(L, 3);
	off_t src_offset = (off_t)luaL_checknumber(L, 4);
	off_t length = (off_t)luaL_checknumber(L, 5);
	
	int err = bless_buffer_insert(buf, offset, src, src_offset, length);
	if (err && should_throw_errors(L))
		return luaL_error(L, "Error inserting data in buffer: %s", strerror(err));
	
	lua_pushnumber(L, err);

	return 1;
}

static int buffer_lua_delete(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	off_t offset = (off_t)luaL_checknumber(L, 2);
	off_t length = (off_t)luaL_checknumber(L, 3);
	
	int err = bless_buffer_delete(buf, offset, length);
	if (err && should_throw_errors(L))
		return luaL_error(L, "Error deleting data from buffer: %s", strerror(err));
	
	lua_pushnumber(L, err);

	return 1;
}

static int buffer_lua_read(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	off_t offset = (off_t)luaL_checknumber(L, 2);
	struct bytearray *ba = to_bytearray(L, 3);
	size_t dst_offset = (size_t)luaL_checknumber(L, 4);
	size_t length = (size_t)luaL_checknumber(L, 5);
	
	luaL_argcheck(L, dst_offset < ba->length, 4,
			"offset out of range");
	luaL_argcheck(L, dst_offset + length <= ba->length, 5,
			"length out of range");
	
	int err = bless_buffer_read(buf, offset, ba->data, dst_offset, length);
	if (err && should_throw_errors(L))
		return luaL_error(L, "Error reading data from buffer: %s", strerror(err));
	
	lua_pushnumber(L, err);

	return 1;
}

static int buffer_lua_undo(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	
	int err = bless_buffer_undo(buf);
	if (err && should_throw_errors(L))
		return luaL_error(L, "Error undoing operation in buffer: %s", strerror(err));
	
	lua_pushnumber(L, err);

	return 1;
}

static int buffer_lua_redo(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	
	int err = bless_buffer_redo(buf);
	if (err && should_throw_errors(L))
		return luaL_error(L, "Error redoing operation in buffer: %s", strerror(err));
	
	lua_pushnumber(L, err);

	return 1;
}

static int buffer_lua_begin_multi_action(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	
	int err = bless_buffer_begin_multi_action(buf);
	if (err && should_throw_errors(L))
		return luaL_error(L, "%s", strerror(err));
	
	lua_pushnumber(L, err);

	return 1;
}

static int buffer_lua_end_multi_action(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	
	int err = bless_buffer_end_multi_action(buf);
	if (err && should_throw_errors(L))
		return luaL_error(L, "%s", strerror(err));
	
	lua_pushnumber(L, err);

	return 1;
}

static int buffer_lua_can_undo(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	int can_undo = 0;
	
	int err = bless_buffer_can_undo(buf, &can_undo);
	if (err && should_throw_errors(L))
		return luaL_error(L, "%s", strerror(err));
	
	lua_pushnumber(L, err);
	lua_pushnumber(L, can_undo);

	return 2;
}

static int buffer_lua_can_redo(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	int can_redo = 0;
	
	int err = bless_buffer_can_redo(buf, &can_redo);
	if (err && should_throw_errors(L))
		return luaL_error(L, "%s", strerror(err));
	
	lua_pushnumber(L, err);
	lua_pushnumber(L, can_redo);

	return 2;
}

static int buffer_lua_set_option(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	int option = luaL_checkint(L, 2);
	char *value = (char *)luaL_checkstring(L, 3);
	
	int err = bless_buffer_set_option(buf, option, value);
	if (err && should_throw_errors(L))
		return luaL_error(L, "%s", strerror(err));
	
	lua_pushnumber(L, err);

	return 1;
}

static int buffer_lua_get_option(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	int option = luaL_checkint(L, 2);
	char *value;
	
	int err = bless_buffer_get_option(buf, &value, option);
	if (err && should_throw_errors(L))
		return luaL_error(L, "%s", strerror(err));
	
	lua_pushnumber(L, err);
	lua_pushstring(L, value);

	return 2;
}

static int buffer_lua_get_size(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	off_t size = 0;
	
	int err = bless_buffer_get_size(buf, &size);
	if (err && should_throw_errors(L))
		return luaL_error(L, "%s", strerror(err));
	
	lua_pushnumber(L, err);
	lua_pushnumber(L, size);

	return 2;
}

static int buffer_lua_gc(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	if (buf != NULL) {
		/* Get reference count */
		lua_getfenv(L, 1);
		lua_pushliteral(L, "refcount");
		lua_rawget(L, -2);
		int ref = lua_tointeger(L, -1);
		
		if (ref == 0)
			bless_buffer_free(buf);
	}
	
	return 0;
}

static int buffer_lua_len(lua_State *L)
{
	bless_buffer_t *buf = to_buffer(L, 1);
	off_t size = 0;
	
	int err = bless_buffer_get_size(buf, &size);
	if (err)
		return luaL_error(L, "%s", strerror(err));
	
	lua_pushnumber(L, size);

	return 1;
}

static const luaL_reg buffer_methods[] = {
	{"new", buffer_lua_new},
	{"save", buffer_lua_save},
	{"append", buffer_lua_append},
	{"insert", buffer_lua_insert},
	{"delete", buffer_lua_delete},
	{"read", buffer_lua_read},
	{"save", buffer_lua_save},
	{"undo", buffer_lua_undo},
	{"redo", buffer_lua_redo},
	{"begin_multi_action", buffer_lua_begin_multi_action},
	{"end_multi_action", buffer_lua_end_multi_action},
	{"can_undo", buffer_lua_can_undo},
	{"can_redo", buffer_lua_can_redo},
	{"get_size", buffer_lua_get_size},
	{"set_option", buffer_lua_set_option},
	{"get_option", buffer_lua_get_option},
	{NULL, NULL}
};

static const luaL_reg buffer_meta[] = {
	{"__gc",  buffer_lua_gc},
	{"__len", buffer_lua_len},
	{0, 0}
};

#define LUAOPEN_BLESS_BUFFER_AGAIN(MAJOR, MINOR) luaopen_bless_buffer_##MAJOR##_##MINOR
#define LUAOPEN_BLESS_BUFFER(MAJOR, MINOR) LUAOPEN_BLESS_BUFFER_AGAIN(MAJOR, MINOR)

/*
 * Register the bless_buffer bindings
 */
LUALIB_API int LUAOPEN_BLESS_BUFFER(LIBBLS_VERSION_MAJOR, LIBBLS_VERSION_MINOR)(lua_State *L)
{
	int bless_table_exists = 0;
	
	bless_table_exists = push_global_bless(L);
	
	/* create bless table if it doesn't exist */
	if (!bless_table_exists)
		lua_newtable(L);
	
	/*
	 * Register buffer source bindings.
	 */
	
	/* buffer_source table */
	lua_newtable(L);
	
	/* fill in buffer_source method table */
	luaL_register(L, NULL, buffer_source_methods);
	
	/* fill in buffer_source metatable */
	luaL_newmetatable(L, "bless.buffer_source");
	luaL_register(L, NULL, buffer_source_meta);
	
	/* metatable.__index = methods */	
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	
	/* metatable.__metatable = methods */	
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	
	lua_pop(L, 1);
	
	/* bless.buffer_source = buffer_source table */
	lua_pushliteral(L, "buffer_source");
	lua_pushvalue(L, -2);
	lua_rawset(L, -4);
	lua_pop(L, 1);
	
	/*
	 * Register buffer bindings.
	 */
	
	/* buffer table */
	lua_newtable(L);
	
	/* fill in buffer method table */
	luaL_register(L, NULL, buffer_methods);
	
	/* fill in buffer_source metatable */
	luaL_newmetatable(L, "bless.buffer");
	luaL_register(L, NULL, buffer_meta);
	
	/* metatable.__index = methods */	
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	
	/* metatable.__metatable = methods */	
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	
	lua_pop(L, 1);
	
	/* fill in enumeration */
	lua_pushinteger(L, BLESS_BUF_TMP_DIR);
	lua_setfield(L, -2, "TMP_DIR");
	lua_pushinteger(L, BLESS_BUF_UNDO_LIMIT);
	lua_setfield(L, -2, "UNDO_LIMIT");
	lua_pushinteger(L, BLESS_BUF_UNDO_AFTER_SAVE);
	lua_setfield(L, -2, "UNDO_AFTER_SAVE");
	
	/* bless.buffer = buffer table */
	lua_pushliteral(L, "buffer");
	lua_pushvalue(L, -2);
	lua_rawset(L, -4);
	lua_pop(L, 1);
	
	/*
	 * Register bytearray bindings.
	 */
	
	/* bytearray table */
	lua_newtable(L);
	
	/* fill in bytearray method table */
	luaL_register(L, NULL, bytearray_methods);
	
	/* fill in bytearray metatable */
	luaL_newmetatable(L, "bless.bytearray");
	luaL_register(L, NULL, bytearray_meta);
	
	/* metatable.__metatable = methods */	
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	
	lua_pop(L, 1);
	
	/* bless.bytearray = bytearray table */
	lua_pushliteral(L, "bytearray");
	lua_pushvalue(L, -2);
	lua_rawset(L, -4);
	lua_pop(L, 1);
	
	/* Register bless table if needed */
	if (!bless_table_exists) {
		lua_pushvalue(L, -1);
		lua_setglobal(L, "bless");
	}
	
	return 1;
}

#pragma GCC visibility pop
