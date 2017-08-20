
#include "util_lua.h"
#include "log_thread.h"
#include "ringbuf.h"
#include "util_clock.h"
#include "zone.h"
#include "zone_thread.h"

#define LUA_TRACEBACK_INDEX 1
#define LUA_ZONE_THREAD_SYSTEM_INDEX 2
#define LUA_ZONE_THREAD_SYSTEM_EVENT_FUNC_INDEX 3

#define LUA_ZONE_THREAD_SYSTEM_SCRIPT_PATH "script/sys/zone_thread/system.lua"

lua_State* zlua_create(RingBuf* logQueue, int logId)
{
    lua_State* L = luaL_newstate();
    int rc;
    
    if (!L) return NULL;
    
    luaL_openlibs(L);
    
    /* debug.traceback is kept at index 1 on the lua stack at all times */
    lua_getglobal(L, "debug");
    lua_getfield(L, LUA_TRACEBACK_INDEX, "traceback");
    lua_replace(L, LUA_TRACEBACK_INDEX);
    
    /* Seed the RNG */
    lua_getglobal(L, "math");
    lua_getfield(L, -1, "randomseed");
    lua_pushinteger(L, (lua_Integer)clock_microseconds());
    
    rc = zlua_call(L, 1, 0, logQueue, logId);
    if (rc) goto fail;
    
    lua_pop(L, 1); /* Pop math */
    
    /* Add the script directory to the package path */
    rc = zlua_run_string(L, 0, logQueue, logId,
        "package.path = package.path .. ';script/?.lua;'");
    if (rc) goto fail;
    
    return L;
    
fail:
    zlua_destroy(L);
    return NULL;
}

lua_State* zlua_destroy(lua_State* L)
{
    if (L)
    {
        lua_close(L);
    }
    
    return NULL;
}

static void zlua_log(RingBuf* logQueue, int logId, const char* funcName, const char* str)
{
    if (logQueue)
    {
        log_writef(logQueue, logId, "%s: lua error with stacktrace:\n%s", funcName, str);
    }
}

int zlua_call(lua_State* L, int numArgs, int numReturns, RingBuf* logQueue, int logId)
{
    if (lua_pcall(L, numArgs, numReturns, LUA_TRACEBACK_INDEX))
    {
        zlua_log(logQueue, logId, FUNC_NAME, lua_tostring(L, -1));
        lua_pop(L, 1);
        return ERR_Lua;
    }
    
    return ERR_None;
}

int zlua_script(lua_State* L, const char* path, int numReturns, RingBuf* logQueue, int logId)
{
    if (luaL_loadfile(L, path) || lua_pcall(L, 0, numReturns, LUA_TRACEBACK_INDEX))
    {
        zlua_log(logQueue, logId, FUNC_NAME, lua_tostring(L, -1));
        lua_pop(L, 1);
        return ERR_Lua;
    }
    
    return ERR_None;
}

int zlua_run_string(lua_State* L, int numReturns, RingBuf* logQueue, int logId, const char* luaString)
{
    if (luaL_loadstring(L, luaString) || lua_pcall(L, 0, numReturns, LUA_TRACEBACK_INDEX))
    {
        zlua_log(logQueue, logId, FUNC_NAME, lua_tostring(L, -1));
        lua_pop(L, 1);
        return ERR_Lua;
    }
    
    return ERR_None;
}

static void zlua_pushfunc(lua_State* L, const char* funcName)
{
    lua_getfield(L, LUA_ZONE_THREAD_SYSTEM_INDEX, funcName);
    
    assert(lua_isfunction(L, -1));
}

void zlua_timer_callback(lua_State* L, int luaIndex)
{
    zlua_pushfunc(L, "timerCallback");
    lua_pushinteger(L, luaIndex);
    zlua_call(L, 1, 0, NULL, 0);
}

int zlua_init_zone_thread(lua_State* L, ZoneThread* zt)
{
    RingBuf* logQueue = zt_get_log_queue(zt);
    int logId = zt_get_log_id(zt);
    int rc;
    
    /* Run the system script and keep its return value at LUA_ZONE_THREAD_SYSTEM_INDEX on the lua stack */
    rc = zlua_script(L, LUA_ZONE_THREAD_SYSTEM_SCRIPT_PATH, 1, logQueue, logId);
    if (rc) goto ret;
    
    /* Create the lua-side ZoneThread object */
    zlua_pushfunc(L, "createZoneThread");
    lua_pushlightuserdata(L, zt);
    rc = zlua_call(L, 1, 0, logQueue, logId);
    if (rc) goto ret;
    
ret:
    return rc;
}

int zlua_init_zone(lua_State* L, Zone* zone)
{
    RingBuf* logQueue = zone_log_queue(zone);
    int logId = zone_log_id(zone);
    int luaIndex;
    int rc;
    
    zlua_pushfunc(L, "createZone");
    lua_pushlightuserdata(L, zone);
    rc = zlua_call(L, 1, 1, logQueue, logId);
    if (rc) goto ret;
    
    luaIndex = lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    zone_set_lua_index(zone, luaIndex);
    
ret:
    return rc;
}

int zlua_deinit_zone(lua_State* L, Zone* zone)
{
    RingBuf* logQueue = zone_log_queue(zone);
    int logId = zone_log_id(zone);
    
    zlua_pushfunc(L, "removeZone");
    lua_pushinteger(L, zone_lua_index(zone));
    return zlua_call(L, 1, 0, logQueue, logId);
}
