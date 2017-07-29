
#ifndef DEFINE_H
#define DEFINE_H

#if defined(_WIN32) || defined(WIN32)
# define PLATFORM_WINDOWS
#else
# define PLATFORM_POSIX
# define PLATFORM_LINUX
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

#ifdef PLATFORM_WINDOWS
# include <winsock2.h>
# include <windows.h>
# include <ws2tcpip.h>
# include "win32_posix_define.h"
#else
# include <stdatomic.h>
# include <errno.h>
# include <inttypes.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <dirent.h>
#endif

#include "enum_err.h"

#ifdef PLATFORM_WINDOWS
# ifdef __cplusplus
#  define ZEQ_API extern "C" __declspec(dllexport)
# else
#  define ZEQ_API __declspec(dllexport)
# endif
#else
# define ZEQ_API extern
#endif

typedef uint8_t byte;

#ifndef __cplusplus
typedef int8_t bool;

# define true 1
# define false 0
#endif

#define sizeof_field(type, name) sizeof(((type*)0)->name)
#define free_if_exists(ptr) do { if ((ptr)) { free((ptr)); (ptr) = NULL; } } while(0)

#endif/*DEFINE_H*/
