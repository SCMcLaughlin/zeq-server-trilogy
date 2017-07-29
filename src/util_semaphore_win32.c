
#include "util_semaphore.h"

int semaphore_init(Semaphore* ptr)
{
    *ptr = CreateSemaphore(NULL, 0, INT_MAX, NULL);
    return (*ptr == NULL) ? ERR_CouldNotInit : ERR_None;
}

int semaphore_deinit(Semaphore* ptr)
{
    HANDLE h = *ptr;
    *ptr = NULL;
    
    return (h && !CloseHandle(h)) ? ERR_Semaphore : ERR_None;
}

int semaphore_wait_win32(Semaphore ptr)
{
    return WaitForSingleObject(ptr, INFINITE) ? ERR_Semaphore : ERR_None;
}

int semaphore_try_wait_win32(Semaphore ptr)
{
    DWORD ret = WaitForSingleObject(ptr, 0);
    int rc;
    
    switch (ret)
    {
    case WAIT_OBJECT_0:
        rc = ERR_None;
        break;

    case WAIT_TIMEOUT:
        rc = ERR_WouldBlock;
        break;

    default:
        rc = ERR_Semaphore;
        break;
    }

    return rc;
}

int semaphore_wait_with_timeout_win32(Semaphore ptr, uint32_t timeoutMs)
{
    DWORD ret = WaitForSingleObject(ptr, timeoutMs);
    int rc;

    switch (ret)
    {
    case WAIT_OBJECT_0:
        rc = ERR_None;
        break;

    case WAIT_TIMEOUT:
        rc = ERR_WouldBlock;
        break;

    default:
        rc = ERR_Semaphore;
        break;
    }

    return rc;
}

int semaphore_trigger_win32(Semaphore ptr)
{
    return ReleaseSemaphore(ptr, 1, NULL) ? ERR_None : ERR_Semaphore;
}
