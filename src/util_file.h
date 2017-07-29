
#ifndef UTIL_FILE_H
#define UTIL_FILE_H

#include "define.h"

#define fwrite_literal(fp, str) fwrite(str, sizeof(char), sizeof(str) - 1, (fp))

#endif/*UTIL_FILE_H*/
