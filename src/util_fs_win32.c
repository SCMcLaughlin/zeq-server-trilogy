
#include "util_fs.h"
#include "util_clock.h"

int fs_modtime(const char* path, uint64_t* outSeconds)
{
    HANDLE file;
    FILETIME ft;
    ULARGE_INTEGER ul;
    int rc = ERR_None;

    file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (!file)
    {
        rc = ERR_NotFound;
        goto fail;
    }

    if (!GetFileTime(file, NULL, NULL, &ft))
    {
        rc = ERR_Generic;
        goto fail_close;
    }

    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;

#define WINDOWS_TICK 10000000
#define SEC_TO_UNIX_EPOCH 11644473600LL

    *outSeconds = ul.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH;

#undef WINDOWS_TICK
#undef SEC_TO_UNIX_EPOCH

fail_close:
    CloseHandle(file);
fail:
    return rc;
}
