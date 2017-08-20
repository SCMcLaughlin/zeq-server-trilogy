
#ifndef TIMER_H
#define TIMER_H

#include "define.h"
#include "util_lua.h"

typedef struct Timer Timer;

typedef struct {
    uint32_t    timerCount;
    uint32_t    triggeredCount;
    uint32_t    triggeredCap;
    uint64_t*   triggerTimes;
    Timer**     timerObjects;
    uint32_t*   triggeredIndices;
} TimerPool;

typedef void(*TimerCallback)(TimerPool* pool, Timer* timer);

struct ZoneThread;

void timer_pool_init(TimerPool* pool);
void timer_pool_deinit(TimerPool* pool);

int timer_pool_execute_callbacks(TimerPool* pool);

/* POSIX defines timer_create(), so we sidestep our usual naming scheme in this one instance... */
Timer* timer_new(TimerPool* pool, uint32_t periodMs, TimerCallback callback, void* userdata, bool start);
ZEQ_API Timer* timer_destroy(TimerPool* pool, Timer* timer);

ZEQ_API Timer* timer_new_lua(lua_State* L, TimerPool* pool, uint32_t periodMs);

ZEQ_API int timer_stop(TimerPool* pool, Timer* timer);
ZEQ_API int timer_restart(TimerPool* pool, Timer* timer);
#define timer_start(pool, timer) (timer_restart((pool), (timer)))

void* timer_userdata(Timer* timer);
#define timer_userdata_type(timer, type) (type*)timer_userdata((timer))

ZEQ_API void timer_set_lua_index(Timer* timer, int luaIndex);
ZEQ_API int timer_lua_index(Timer* timer);

#endif/*TIMER_H*/
