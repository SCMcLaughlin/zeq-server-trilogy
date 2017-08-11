
#ifndef ENUM_ERR_H
#define ENUM_ERR_H

enum EnumError {
    ERR_None,
    ERR_Generic,
    ERR_CouldNotInit,
    ERR_CouldNotOpen,
    ERR_Invalid,
    ERR_OutOfMemory,
    ERR_OutOfSpace,
    ERR_Timeout,
    ERR_IO,
    ERR_Again,
    ERR_NotInitialized,
    ERR_CouldNotCreate,
    ERR_Semaphore,
    ERR_FileOperation,
    ERR_Fatal,
    ERR_WouldBlock,
    ERR_OutOfBounds,
    ERR_Compression,
    ERR_Library,
    ERR_Lua,
    ERR_EndOfFile,
    ERR_NotFound,
    ERR_Mismatch,
    ERR_Cancel,
    ERR_COUNT
};

#endif/*ENUM_ERR_H*/
