
#ifndef TIMER_H
#define TIMER_H

#include "define.h"

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

void timer_pool_init(TimerPool* pool);
void timer_pool_deinit(TimerPool* pool);

int timer_pool_execute_callbacks(TimerPool* pool);

/* POSIX defines timer_create(), so we sidestep our usual naming scheme in this one instance... */
Timer* timer_new(TimerPool* pool, uint32_t periodMs, TimerCallback callback, void* userdata, bool start);
Timer* timer_destroy(TimerPool* pool, Timer* timer);

int timer_stop(TimerPool* pool, Timer* timer);
int timer_restart(TimerPool* pool, Timer* timer);
#define timer_start(pool, timer) (timer_restart((pool), (timer)))

#endif/*TIMER_H*/
