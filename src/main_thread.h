
#ifndef MAIN_THREAD_H
#define MAIN_THREAD_H

#include "define.h"

typedef struct MainThread MainThread;

MainThread* mt_create();
MainThread* mt_destroy(MainThread* mt);

void mt_main_loop(MainThread* mt);

#endif/*MAIN_THREAD_H*/
