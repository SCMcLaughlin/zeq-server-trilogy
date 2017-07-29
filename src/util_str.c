
#include "util_str.h"

int str_len_bounded(const char* str, int max)
{
    int i = 0;
    
    while (i < max)
    {
        int c = *str++;
        
        if (c == 0) break;
        
        i++;
    }
    
    return i;
}

int str_len_overflow(const char* str, int max)
{
    int i = 0;
    
    while (i < max)
    {
        int c = *str++;
        
        if (c == 0)
            return i;
        
        i++;
    }
    
    return -1;
}
