
#include "util_fs.h"
#include "util_clock.h"

int fs_modtime(const char* path, uint64_t* outSeconds)
{
    struct stat st;
    
    if (stat(path, &st))
        return ERR_NotFound;
    
    *outSeconds = (uint64_t)st.st_mtime;
    return ERR_None;
}
