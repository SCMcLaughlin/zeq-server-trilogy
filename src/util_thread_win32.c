
#include "define.h"
#include "util_thread.h"
#include "util_alloc.h"

typedef struct {
    ThreadProc  func;
    void*       userdata;
} ThreadCreate;

static DWORD WINAPI thread_proc_wrapper(void* ptr)
{
    ThreadCreate* tc = (ThreadCreate*)ptr;
    ThreadProc func = tc->func;
    void* userdata = tc->userdata;
    
    free(tc);
    func(userdata);
    return 0;
}

int thread_start(ThreadProc func, void* userdata)
{
    ThreadCreate* tc = alloc_type(ThreadCreate);
    HANDLE handle;
    
    if (!tc)
    {
        return ERR_OutOfMemory;
    }
    
    tc->func = func;
    tc->userdata = userdata;
    
    handle = CreateThread(NULL, 0, thread_proc_wrapper, tc, 0, NULL);
    
    if (!handle)
    {
        free(tc);
        return ERR_CouldNotInit;
    }
    
    CloseHandle(handle);
    return ERR_None;
}
