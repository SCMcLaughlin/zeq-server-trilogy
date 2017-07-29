
#ifndef DB_ZPACKET_H
#define DB_ZPACKET_H

#include "define.h"
#include "buffer.h"
#include "ringbuf.h"
#include "login_crypto.h"

/* Query structs */

typedef struct  {
    void*           client;
    StaticBuffer*   accountName;
} DBQ_LoginCredentials;

typedef struct {
    int         dbId;
    int         queryId;    /* User-supplied tracking value, not used internally by the DB system */
    int         zop;
    RingBuf*    replyQueue;
    union {
        DBQ_LoginCredentials    qLoginCredentials;
    };
} DB_ZQuery;

/* Result structs */

typedef struct {
    void*   client;
    int64_t loginId;
    byte    passwordHash[LOGIN_CRYPTO_HASH_SIZE];
    byte    salt[LOGIN_CRYPTO_SALT_SIZE];
} DBR_LoginCredentials;

typedef struct {
    int         queryId;    /* User-supplied tracking value, not used internally by the DB system */
    bool        hadError;
    union {
        DBR_LoginCredentials    rLoginCredentials;
    };
} DB_ZResult;

/* Other structs */

typedef struct {
    int             dbId;
    RingBuf*        errorQueue; /* One-time error report if the open fails */
    StaticBuffer*   path;
    StaticBuffer*   schemaPath;
} DB_ZOpen;

typedef union {
    DB_ZOpen    zOpen;
    DB_ZQuery   zQuery;
    DB_ZResult  zResult;
} DB_ZPacket;

#endif/*DB_ZPACKET_H*/
