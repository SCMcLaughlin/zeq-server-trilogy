
#include "util_lua.h"
#include "log_thread.h"
#include "ringbuf.h"
#include "util_clock.h"

#define LUA_TRACEBACK_INDEX 1

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
        "package.path = package.path .. ';script/?.lua'");
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
