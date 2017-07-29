
#ifndef UTIL_THREAD_H
#define UTIL_THREAD_H

typedef void(*ThreadProc)(void* userdata);

int thread_start(ThreadProc func, void* userdata);

#endif/*UTIL_THREAD_H*/
