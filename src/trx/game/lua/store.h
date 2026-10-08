// The tables a script keeps across a save.
//
// Two scopes: the level store is emptied when a level loads, and the game
// store lasts a playthrough. Both are plain Lua tables that hold numbers,
// strings, booleans and tables of those. A value of any other type is dropped
// when the game is saved, and the key it sat at is named in the log.
//
// A table is written once and named by an id, and every later sighting of it
// writes the id instead. A table stored in two places therefore loads as one
// table, and a table that holds itself loads whole.
#pragma once

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/result.h>

void LUA_Store_ClearLevel(void);
void LUA_Store_ClearGame(void);

void LUA_Store_Dump(JSON_WRITE_IO *io);
RESULT LUA_Store_Load(JSON_READ_IO *io);

// Spelled out rather than taken from lua.h, which the save code does not build
// against.
typedef struct lua_State lua_State;

// Push the level store and the game store, the tables trxc.store hands a
// script.
void LUA_Store_PushLevelTable(lua_State *L);
void LUA_Store_PushGameTable(lua_State *L);
