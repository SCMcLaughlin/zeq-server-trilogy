
#include "util_semaphore.h"

int semaphore_init(Semaphore* ptr)
{
    return sem_init(ptr, 0, 0) ? ERR_CouldNotInit : ERR_None;
}

int semaphore_deinit(Semaphore* ptr)
{
    return sem_destroy(ptr) ? ERR_Invalid : ERR_None;
}

int semaphore_wait(Semaphore* ptr)
{
    return sem_wait(ptr) ? ERR_Semaphore : ERR_None;
}

int semaphore_try_wait(Semaphore* ptr)
{
    if (sem_trywait(ptr))
    {
        int err = errno;
        return (err == EAGAIN) ? ERR_WouldBlock : ERR_Semaphore;
    }
    
    return ERR_None;
}

int semaphore_wait_with_timeout(Semaphore* ptr, uint32_t timeoutMs)
{
    struct timespec t;
    t.tv_sec = timeoutMs / 1000;
    t.tv_nsec = (timeoutMs % 1000) * 1000000;
    
    if (sem_timedwait(ptr, &t))
    {
        int err = errno;
        return (err == EAGAIN) ? ERR_WouldBlock : ERR_Semaphore;
    }
    
    return ERR_None;
}

int semaphore_trigger(Semaphore* ptr)
{
    /*
    sem_post() has two possible errors:
    EINVAL: the semaphore is invalid
    EOVERFLOW: the semaphore's int would overflow
    
    We don't care about EOVERFLOW; it will still be waking threads
    as long as the value is anything above 0, which is all we want
    */
    if (sem_post(ptr))
    {
        int err = errno;
        if (err == EINVAL)
            return ERR_Invalid;
    }
    
    return ERR_None;
}
