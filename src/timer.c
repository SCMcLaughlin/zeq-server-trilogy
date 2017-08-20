
#include "timer.h"
#include "bit.h"
#include "util_alloc.h"
#include "util_clock.h"
#include "zone_thread.h"

#define TIMER_POOL_INVALID_INDEX 0xffffffff

struct Timer {
    uint32_t        poolIndex;
    uint32_t        periodMs;
    int             luaIndex;
    TimerCallback   callback;
    void*           userdata;
};

void timer_pool_init(TimerPool* pool)
{
    pool->timerCount = 0;
    pool->triggeredCap = 0;
    pool->triggerTimes = NULL;
    pool->timerObjects = NULL;
    pool->triggeredIndices = NULL;
}

void timer_pool_deinit(TimerPool* pool)
{
    pool->timerCount = 0;
    pool->triggeredCap = 0;
    free_if_exists(pool->triggerTimes);
    free_if_exists(pool->timerObjects);
    free_if_exists(pool->triggeredIndices);
}

static void timer_pool_swap_and_pop(TimerPool* pool, uint32_t index)
{
    uint32_t back = --pool->timerCount;
    
    if (back != index)
    {
        Timer* timer = pool->timerObjects[back];
        pool->timerObjects[index] = timer;
        pool->triggerTimes[index] = pool->triggerTimes[back];
        timer->poolIndex = index;
    }
}

static int timer_pool_add_trigger_index(TimerPool* pool, uint32_t index)
{
    uint32_t count = pool->triggeredCount;
    uint32_t cap = pool->triggeredCap;
    
    if (count >= cap)
    {
        uint32_t* array;
        
        cap = (cap == 0) ? 1 : cap * 2;
        array = realloc_array_type(pool->triggeredIndices, cap, uint32_t);
        
        if (!array) return false;
        
        pool->triggeredCap = cap;
        pool->triggeredIndices = array;
    }
    
    pool->triggeredIndices[count] = index;
    pool->triggeredCount = count + 1;
    return true;
}

int timer_pool_execute_callbacks(TimerPool* pool)
{
    uint32_t i = 0;
    uint32_t n = pool->timerCount;
    uint64_t* triggerTimes = pool->triggerTimes;
    uint64_t curTime = clock_milliseconds();
    
    /* Guarantee: nothing in this loop will cause triggerTimes to relocate */
    while (i < n)
    {
        uint64_t triggerTime = triggerTimes[i];
        
        /* Is the 'dead' bit set? */
        if (bit_get64(triggerTime, 63))
        {
            timer_pool_swap_and_pop(pool, i);
            n--;
            continue;
        }
        
        /* Don't count the 'dead' bit as part of the trigger time */
        if (curTime >= (triggerTime & bit_mask64(63)) && !timer_pool_add_trigger_index(pool, i))
        {
            return ERR_OutOfMemory;
        }
        
        i++;
    }
    
    n = pool->triggeredCount;
    pool->triggeredCount = 0;
    
    if (n > 0)
    {
        uint32_t* triggered = pool->triggeredIndices;
        
        /* This loop may cause timerObjects or triggerTimes to relocate */
        for (i = 0; i < n; i++)
        {
            Timer* timer = pool->timerObjects[triggered[i]];
    
            pool->triggerTimes[i] += timer->periodMs;

            if (timer->callback)
                timer->callback(pool, timer);
        }
    }
    
    return ERR_None;
}

Timer* timer_new(TimerPool* pool, uint32_t periodMs, TimerCallback callback, void* userdata, bool start)
{
    Timer* timer = alloc_type(Timer);
    
    if (!timer) return NULL;
    
    timer->poolIndex = TIMER_POOL_INVALID_INDEX;
    timer->periodMs = periodMs;
    timer->luaIndex = 0;
    timer->callback = callback;
    timer->userdata = userdata;
    
    if (start && ERR_None != timer_start(pool, timer))
    {
        free(timer);
        return NULL;
    }
    
    return timer;
}

Timer* timer_destroy(TimerPool* pool, Timer* timer)
{
    if (timer)
    {
        timer_stop(pool, timer);
        free(timer);
    }
    
    return NULL;
}

static void timer_lua_callback(TimerPool* pool, Timer* timer)
{
    lua_State* L = timer_userdata_type(timer, lua_State);
    
    (void)pool;
    
    zlua_timer_callback(L, timer->luaIndex);
}

Timer* timer_new_lua(lua_State* L, TimerPool* pool, uint32_t periodMs)
{
    return timer_new(pool, periodMs, timer_lua_callback, L, true);
}

int timer_stop(TimerPool* pool, Timer* timer)
{
    uint32_t index = timer->poolIndex;
    int rc;
    
    if (index != TIMER_POOL_INVALID_INDEX)
    {
        timer->poolIndex = TIMER_POOL_INVALID_INDEX;
        /* The 63rd bit (counting from 0) is the 'dead' flag bit */
        bit_set64(pool->triggerTimes[index], 63);
        rc = ERR_None;
    }
    else
    {
        rc = ERR_Invalid;
    }
    
    return rc;
}

static int timer_pool_start_timer(TimerPool* pool, Timer* timer)
{
    uint64_t curTime = clock_milliseconds();
    uint32_t index = pool->timerCount;
    
    if (bit_is_pow2_or_zero(index))
    {
        uint32_t cap = (index == 0) ? 1 : index * 2;
        uint64_t* triggerTimes;
        Timer** timerObjects;
        
        triggerTimes = realloc_array_type(pool->triggerTimes, cap, uint64_t);
        if (!triggerTimes) goto fail;
        pool->triggerTimes = triggerTimes;
        
        timerObjects = realloc_array_type(pool->timerObjects, cap, Timer*);
        if (!timerObjects) goto fail;
        pool->timerObjects = timerObjects;
    }
    
    pool->timerCount = index + 1;
    pool->timerObjects[index] = timer;
    pool->triggerTimes[index] = curTime + timer->periodMs;
    
    timer->poolIndex = index;
    
    return ERR_None;
fail:
    return ERR_OutOfMemory;
}

int timer_restart(TimerPool* pool, Timer* timer)
{
    uint32_t index = timer->poolIndex;
    int rc;
    
    if (index == TIMER_POOL_INVALID_INDEX)
    {
        rc = timer_pool_start_timer(pool, timer);
    }
    else
    {
        pool->triggerTimes[index] = clock_milliseconds() + timer->periodMs;
        rc = ERR_None;
    }
    
    return rc;
}

void* timer_userdata(Timer* timer)
{
    return timer->userdata;
}

void timer_set_lua_index(Timer* timer, int luaIndex)
{
    timer->luaIndex = luaIndex;
}

int timer_lua_index(Timer* timer)
{
    return timer->luaIndex;
}
