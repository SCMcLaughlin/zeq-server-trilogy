
#ifndef DB_ZPACKET_H
#define DB_ZPACKET_H

#include "define.h"
#include "buffer.h"
#include "char_select_data.h"
#include "client_load_data.h"
#include "guild.h"
#include "item_proto.h"
#include "ringbuf.h"
#include "login_crypto.h"

/* == Query structs == */

/* LoginThread */

typedef struct  {
    void*           client;
    StaticBuffer*   accountName;
} DBQ_LoginCredentials;

typedef struct {
    void*           client;
    StaticBuffer*   accountName;
    byte            passwordHash[LOGIN_CRYPTO_HASH_SIZE];
    byte            salt[LOGIN_CRYPTO_SALT_SIZE];
} DBQ_LoginNewAccount;

/* CharSelectThread */

typedef struct {
    void*       client;
    int64_t     accountId;
} DBQ_CSCharacterInfo;

typedef struct {
    void*           client;
    StaticBuffer*   name;
} DBQ_CSCharacterNameAvailable;

typedef struct {
    void*           client;
    int64_t         accountId;
    CharCreateData* data;
} DBQ_CSCharacterCreate;

typedef struct {
    void*           client;
    int64_t         accountId;
    StaticBuffer*   name;
} DBQ_CSCharacterDelete;

/* MainThread */

typedef struct {
    void*           client;
    int64_t         accountId;
    StaticBuffer*   name;
} DBQ_MainLoadCharacter;

typedef struct {
    int         dbId;
    int         queryId;    /* User-supplied tracking value, not used internally by the DB system */
    int         zop;
    RingBuf*    replyQueue;
    union {
        DBQ_LoginCredentials            qLoginCredentials;
        DBQ_LoginNewAccount             qLoginNewAccount;
        DBQ_CSCharacterInfo             qCSCharacterInfo;
        DBQ_CSCharacterNameAvailable    qCSCharacterNameAvailable;
        DBQ_CSCharacterCreate           qCSCharacterCreate;
        DBQ_CSCharacterDelete           qCSCharacterDelete;
        ItemProtoDbChanges              qMainItemProtoChanges;
        DBQ_MainLoadCharacter           qMainLoadCharacter;
    };
} DB_ZQuery;

/* == Result structs == */

/* MainThread */

typedef struct {
    uint32_t        count;
    uint32_t        highItemId;
    ItemProtoDb*    protos;
} DBR_MainLoadItemProtos;

typedef struct {
    uint32_t    count;
    Guild*      guilds;
} DBR_MainGuildList;

typedef struct {
    void*                       client;
    ClientLoadData_Character*   data;
} DBR_MainLoadCharacter;

/* LoginThread */

typedef struct {
    void*   client;
    int64_t acctId;
    byte    passwordHash[LOGIN_CRYPTO_HASH_SIZE];
    byte    salt[LOGIN_CRYPTO_SALT_SIZE];
    int     status;
    int64_t suspendedUntil;
} DBR_LoginCredentials;

typedef struct {
    void*   client;
    int64_t acctId;
} DBR_LoginNewAccount;

/* CharSelectThread */

typedef struct {
    uint32_t        count;
    void*           client;
    CharSelectData* data;
} DBR_CSCharacterInfo;

typedef struct {
    bool    isNameAvailable;
    void*   client;
} DBR_CSCharacterNameAvailable;

typedef struct {
    void*   client;
} DBR_CSCharacterCreate;

typedef struct {
    int         queryId;    /* User-supplied tracking value, not used internally by the DB system */
    bool        hadError;
    bool        hadErrorUnprocessed;
    union {
        DBR_MainLoadItemProtos          rMainLoadItemProtos;
        DBR_MainGuildList               rMainGuildList;
        DBR_LoginCredentials            rLoginCredentials;
        DBR_LoginNewAccount             rLoginNewAccount;
        DBR_CSCharacterInfo             rCSCharacterInfo;
        DBR_CSCharacterNameAvailable    rCSCharacterNameAvailable;
        DBR_CSCharacterCreate           rCSCharacterCreate;
        DBR_CSCharacterCreate           rCSCharacterDelete;
        DBR_MainLoadCharacter           rMainLoadCharacter;
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
