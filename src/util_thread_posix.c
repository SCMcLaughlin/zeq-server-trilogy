
#include "define.h"
#include "util_thread.h"
#include "util_alloc.h"
#include <pthread.h>

typedef struct {
    ThreadProc  func;
    void*       userdata;
} ThreadCreate;

static void* thread_proc_wrapper(void* ptr)
{
    ThreadCreate* tc = (ThreadCreate*)ptr;
    ThreadProc func = tc->func;
    void* userdata = tc->userdata;
    
    free(tc);
    func(userdata);
    return NULL;
}

int thread_start(ThreadProc func, void* userdata)
{
    ThreadCreate* tc = alloc_type(ThreadCreate);
    pthread_t pthread;
    
    if (!tc) return ERR_OutOfMemory;
    
    tc->func = func;
    tc->userdata = userdata;
    
    if (pthread_create(&pthread, NULL, thread_proc_wrapper, tc) || pthread_detach(pthread))
    {
        free(tc);
        return ERR_CouldNotInit;
    }
    
    return ERR_None;
}
