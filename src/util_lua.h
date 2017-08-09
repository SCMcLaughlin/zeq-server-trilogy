
#ifndef UTIL_LUA_H
#define UTIL_LUA_H

#include "define.h"
#include "item_proto.h"
#include "ringbuf.h"
#include <lua.h>
#include <luaconf.h>
#include <lualib.h>
#include <lauxlib.h>

lua_State* zlua_create(RingBuf* logQueue, int logId);
lua_State* zlua_destroy(lua_State* L);

int zlua_call(lua_State* L, int numArgs, int numReturns, RingBuf* logQueue, int logId);
int zlua_script(lua_State* L, const char* path, int numReturns, RingBuf* logQueue, int logId);
int zlua_run_string(lua_State*, int numReturns, RingBuf* logQueue, int logId, const char* luaString);

int zlua_load_items(lua_State* L, ItemList* itemList, RingBuf* logQueue, int logId);

#endif/*UTIL_LUA_H*/
