
#ifndef UTIL_SEMAPHORE_H
#define UTIL_SEMAPHORE_H

#include "define.h"

#ifdef PLATFORM_WINDOWS
typedef HANDLE Semaphore;

int semaphore_init(Semaphore* sem);
int semaphore_deinit(Semaphore* sem);

int semaphore_wait_win32(Semaphore sem);
int semaphore_try_wait_win32(Semaphore sem);
int semaphore_wait_with_timeout_win32(Semaphore sem, uint32_t timeoutMs);
int semaphore_trigger_win32(Semaphore sem);

# define semaphore_wait(ptr) (semaphore_wait_win32(*(ptr)))
# define semaphore_try_wait(ptr) (semaphore_try_wait_win32(*(ptr)))
# define semaphore_wait_with_timeout(ptr, ms) (semaphore_wait_with_timeout_win32(*(ptr), (ms)))
# define semaphore_trigger(ptr) (semaphore_trigger_win32(*(ptr)))
#else /* PLATFORM_POSIX */
#include <semaphore.h>
typedef sem_t Semaphore;

int semaphore_init(Semaphore* ptr);
int semaphore_deinit(Semaphore* ptr);

int semaphore_wait(Semaphore* ptr);
int semaphore_try_wait(Semaphore* ptr);
int semaphore_wait_with_timeout(Semaphore* ptr, uint32_t timeoutMs);
int semaphore_trigger(Semaphore* ptr);
#endif

#endif/*UTIL_SEMAPHORE_H*/
