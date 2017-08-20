
#ifndef UTIL_LUA_H
#define UTIL_LUA_H

#include "define.h"
#include "item_proto.h"
#include "ringbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <luaconf.h>
#include <lualib.h>
#include <lauxlib.h>

#ifdef __cplusplus
}
#endif

struct ZoneThread;
struct Zone;

lua_State* zlua_create(RingBuf* logQueue, int logId);
lua_State* zlua_destroy(lua_State* L);

int zlua_call(lua_State* L, int numArgs, int numReturns, RingBuf* logQueue, int logId);
int zlua_script(lua_State* L, const char* path, int numReturns, RingBuf* logQueue, int logId);
int zlua_run_string(lua_State* L, int numReturns, RingBuf* logQueue, int logId, const char* luaString);

void zlua_timer_callback(lua_State* L, int luaIndex);

int zlua_init_zone_thread(lua_State* L, struct ZoneThread* zt);

int zlua_init_zone(lua_State* L, struct Zone* zone);
int zlua_deinit_zone(lua_State* L, struct Zone* zone);

#endif/*UTIL_LUA_H*/
