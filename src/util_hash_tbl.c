
#include "util_hash_tbl.h"
#include "hash.h"
#include "util_alloc.h"

#ifdef PLATFORM_WINDOWS
/* data[0]: "nonstandard extension used: zero-sized array in struct/union" */
# pragma warning(disable: 4200)
#endif

typedef struct {
    union {
        StaticBuffer*   keyStr; /* The hash table makes private copies of all keys */
        int64_t         keyInt;
    };
    uint32_t    hash;
    uint32_t    next;
    byte        data[0];
} HashTblEnt;

#define MIN_CAPACITY        4 /* Must be a power of 2 */
#define NEXT_INVALID        0x7fffffff
#define FREE_INDEX_INVALID  0x7fffffff

#define ent_is_empty(ent) ((ent)->keyInt == 0 && (ent)->hash == 0)
#define ent_is_int_key(ent) (((ent)->next & ((uint32_t)(1 << 31))) >> 31)
#define ent_set_int_key(ent) ((ent)->next |= ((uint32_t)(1 << 31)))
#define ent_set_str_key(ent) ((ent)->next &= (((uint32_t)(1 << 31)) - 1))
#define ent_set_next(ent, nx) ((ent)->next = ((nx) | ((ent)->next & ((uint32_t)(1 << 31)))))
#define ent_get_next(ent) ((ent)->next & (((uint32_t)(1 << 31)) - 1))
#define ent_has_next(ent) (ent_get_next(ent) != NEXT_INVALID)

void tbl_init_size(HashTbl* tbl, uint32_t elemSize)
{
    uint32_t entSize = elemSize + sizeof(HashTblEnt);
    
    /* Make sure each HashTblEnt will be a multiple of 8 bytes */
    if (entSize%8 != 0)
        entSize += 8 - entSize%8;
    
    tbl->capacity = 0;
    tbl->elemSize = elemSize;
    tbl->freeIndex = 0;
    tbl->entSize = entSize;
    tbl->data = NULL;
}

static void tbl_free_keys(HashTbl* tbl)
{
    uint32_t entSize = tbl->entSize;
    byte* data = tbl->data;
    uint32_t n = tbl->capacity;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        HashTblEnt* ent = (HashTblEnt*)data;
        
        if (!ent_is_empty(ent) && !ent_is_int_key(ent))
            ent->keyStr = sbuf_drop(ent->keyStr);
        
        data += entSize;
    }
}

void tbl_deinit(HashTbl* tbl, HashTblElemCallback dtor)
{
    if (tbl->data)
    {
        if (dtor)
            tbl_for_each(tbl, dtor);
        
        tbl_free_keys(tbl);
        free(tbl->data);
        
        tbl->capacity = 0;
        tbl->freeIndex = 0;
        tbl->data = NULL;
    }
}

static void tbl_advance_free_index(HashTbl* tbl)
{
    uint32_t freeIndex = tbl->freeIndex;
    uint32_t capacity = tbl->capacity;
    uint32_t size = tbl->entSize;
    byte* data = tbl->data;
    
    while (++freeIndex != capacity)
    {
        HashTblEnt* ent = (HashTblEnt*)&data[freeIndex * size];
        
        if (ent_is_empty(ent))
        {
            tbl->freeIndex = freeIndex;
            return;
        }
    }
    
    tbl->freeIndex = FREE_INDEX_INVALID;
}

static int tbl_alloc_default(HashTbl* tbl)
{
    uint32_t entSize = tbl->entSize;
    byte* data = alloc_bytes(entSize * MIN_CAPACITY);
    uint32_t i;
    
    if (!data) return false;
    
    memset(data, 0, entSize * MIN_CAPACITY);
    
    tbl->capacity = MIN_CAPACITY;
    tbl->data = data;
    
    for (i = 0; i < MIN_CAPACITY; i++)
    {
        HashTblEnt* ent = (HashTblEnt*)&data[entSize * i];
        
        ent->next = NEXT_INVALID;
    }
    
    return true;
}

static int tbl_ent_insert(HashTblEnt* ent, int64_t key, int isIntKey, const void* value, uint32_t hash, uint32_t elemSize)
{
    if (isIntKey)
    {
        ent->keyInt = key;
        ent_set_int_key(ent);
    }
    else
    {
        StaticBuffer* bkey = (StaticBuffer*)key;

        ent->keyStr = bkey;
        ent_set_str_key(ent);
        sbuf_grab(bkey); /* The HashTbl always grabs an inserted string key */
    }
    
    ent->hash = hash;

    if (value && elemSize)
        memcpy(ent->data, value, elemSize);

    return ERR_None;
}

static void tbl_ent_copy(HashTblEnt* newEnt, HashTblEnt* oldEnt, uint32_t elemSize)
{
    newEnt->keyInt = oldEnt->keyInt;
    newEnt->hash = oldEnt->hash;
    if (elemSize)
        memcpy(newEnt->data, oldEnt->data, elemSize);
    
    if (ent_is_int_key(oldEnt))
        ent_set_int_key(newEnt);
    else
        ent_set_str_key(newEnt);
}

static int tbl_realloc(HashTbl* tbl)
{
    uint32_t n = tbl->capacity;
    uint32_t newCap = n * 2;
    uint32_t elemSize = tbl->elemSize;
    uint32_t entSize = tbl->entSize;
    uint32_t initSize = entSize * newCap;
    byte* oldData = tbl->data;
    byte* newData = alloc_bytes(initSize);
    uint32_t newFreeIndex = 0;
    uint32_t i;
    
    if (!newData) return false;
    
    memset(newData, 0, initSize);
    
    for (i = 0; i < newCap; i++)
    {
        HashTblEnt* ent = (HashTblEnt*)&newData[entSize * i];
        
        ent->next = NEXT_INVALID;
    }
    
    newCap--;
    
    for (i = 0; i < n; i++)
    {
        HashTblEnt* oldEnt = (HashTblEnt*)&oldData[entSize * i];
        uint32_t pos = oldEnt->hash & newCap;
        HashTblEnt* newEnt = (HashTblEnt*)&newData[entSize * pos];
        
        if (ent_is_empty(newEnt))
        {
            /* Copy key ptr, hash, value */
            tbl_ent_copy(newEnt, oldEnt, elemSize);
            
            if (pos != newFreeIndex)
                continue;
        }
        else
        {
            uint32_t mainPos = newEnt->hash & newCap;
            
            if (mainPos != pos)
            {
                HashTblEnt* mainEnt;
                memcpy(&newData[entSize * newFreeIndex], newEnt, entSize);
                mainEnt = (HashTblEnt*)&newData[entSize * mainPos];
                
                while (ent_get_next(mainEnt) != pos)
                {
                    mainEnt = (HashTblEnt*)&newData[entSize * ent_get_next(mainEnt)];
                }
                
                ent_set_next(mainEnt, newFreeIndex);
                tbl_ent_copy(newEnt, oldEnt, elemSize);
                ent_set_next(newEnt, FREE_INDEX_INVALID);
            }
            else
            {
                HashTblEnt* freeEnt;
                
                while (ent_has_next(newEnt))
                {
                    newEnt = (HashTblEnt*)&newData[entSize * ent_get_next(newEnt)];
                }
                
                ent_set_next(newEnt, newFreeIndex);
                freeEnt = (HashTblEnt*)&newData[entSize * newFreeIndex];
                tbl_ent_copy(freeEnt, oldEnt, elemSize);
            }
        }
        
        /* If we reach here, advance the new free index */
        for (;;)
        {
            HashTblEnt* ent = (HashTblEnt*)&newData[entSize * (++newFreeIndex)];
            
            if (ent_is_empty(ent))
                break;
        }
    }
    
    free(oldData);
    
    tbl->capacity = newCap + 1;
    tbl->freeIndex = newFreeIndex;
    tbl->data = newData;
    
    return true;
}

static int tbl_set_impl(HashTbl* tbl, int64_t key, int isIntKey, const void* value, uint32_t hash, int updateInPlace)
{
    uint32_t freeIndex;
    uint32_t capMinusOne;
    uint32_t pos;
    uint32_t entSize;
    HashTblEnt* ent;
    
    if (!tbl->data && !tbl_alloc_default(tbl))
        return ERR_OutOfMemory;
    
    freeIndex = tbl->freeIndex;
    capMinusOne = tbl->capacity - 1;
    pos = hash & capMinusOne;
    entSize = tbl->entSize;
    ent = (HashTblEnt*)&tbl->data[pos * entSize];
    
    if (ent_is_empty(ent))
    {
        if (pos == freeIndex)
            tbl_advance_free_index(tbl);
        
        tbl_ent_insert(ent, key, isIntKey, value, hash, tbl->elemSize);
        return ERR_None;
    }
    
    if (freeIndex != FREE_INDEX_INVALID)
    {
        uint32_t mainPos = ent->hash & capMinusOne;
        
        if (mainPos != pos)
        {
            HashTblEnt* mainEnt;
            
            /* Evict this entry to the free index */
            memcpy(&tbl->data[freeIndex * entSize], ent, entSize);
            
            /* Follow this entry's chain to the end, then point the last entry to the free index */
            mainEnt = (HashTblEnt*)&tbl->data[mainPos * entSize];
            
            while (ent_get_next(mainEnt) != pos)
            {
                mainEnt = (HashTblEnt*)&tbl->data[ent_get_next(mainEnt) * entSize];
            }
            
            ent_set_next(mainEnt, freeIndex);
            tbl_advance_free_index(tbl);
            tbl_ent_insert(ent, key, isIntKey, value, hash, tbl->elemSize);
            ent_set_next(ent, FREE_INDEX_INVALID);
            return ERR_None;
        }
        
        /*
            If we reach here, the entry is in its main position
            Check if we already have this key in the table, and abort if so;
            otherwise, follow the chain to the end, link the last entry to the
            free index, and add our new key value pair at the free index
        */
        for (;;)
        {
            if (ent->hash == hash && ent_is_int_key(ent) == (uint32_t)isIntKey)
            {
                if (isIntKey)
                {
                    if (ent->keyInt == key)
                    {
                        if (updateInPlace)
                            goto update;
                        
                        return ERR_Again;
                    }
                }
                else
                {
                    StaticBuffer* bkey = (StaticBuffer*)key;
                    uint32_t len;

                    /* If the key is the exact same Buffer, we don't need to re-grab it, because we assume the HashTbl already owns a reference to it */
                    if (ent->keyStr == bkey)
                    {
                        if (updateInPlace)
                            goto update;
                        return ERR_Again;
                    }

                    len = sbuf_length(bkey);

                    if (sbuf_length(ent->keyStr) == len && strncmp(sbuf_str(ent->keyStr), sbuf_str(bkey), len) == 0)
                    {
                        if (updateInPlace)
                        {
                            /* If this is a different StaticBuffer object, we drop the old one and grab the new one */
                            ent->keyStr = sbuf_drop(ent->keyStr);
                            sbuf_grab(bkey);
                            goto update;
                        }

                        return ERR_Again;
                    }
                }
            }
            
            if (ent_has_next(ent))
                ent = (HashTblEnt*)&tbl->data[ent_get_next(ent) * entSize];
            else
                break;
        }
        
        ent_set_next(ent, freeIndex);
        tbl_advance_free_index(tbl);
        tbl_ent_insert((HashTblEnt*)&tbl->data[freeIndex * entSize], key, isIntKey, value, hash, tbl->elemSize);
        return ERR_None;
        
    update:
        if (value && tbl->elemSize)
            memcpy(ent->data, value, tbl->elemSize);
        return ERR_None;
    }
    
    /* If we reach here, there are no free spaces left in the hash table */
    if (!tbl_realloc(tbl))
        return ERR_OutOfMemory;
    
    return tbl_set_impl(tbl, key, isIntKey, value, hash, updateInPlace);
}

static int tbl_do_set_str(HashTbl* tbl, const char* key, uint32_t len, const void* value, int update)
{
    uint32_t hash;
    StaticBuffer* bkey;
    int rc;
    
    if (!key) return ERR_Invalid;
    
    if (len == 0)
        len = strlen(key);

    hash = hash_str(key, len);
    bkey = sbuf_create(key, len);
    sbuf_grab(bkey);

    if (!bkey) return ERR_OutOfMemory;

    rc = tbl_set_impl(tbl, (int64_t)bkey, false, value, hash, update);

    /*
        If there are no errors, tbl_set_impl() guarantees that it grabbed the bkey; we need to drop the initial reference from buf_create() to balance out.
        If there were errors, we need to free the bkey entirely.
    */
    sbuf_drop(bkey);
    return rc;
}

static int tbl_do_set_int(HashTbl* tbl, int64_t key, const void* value, int update)
{
    uint32_t hash = hash_int64(key);
    return tbl_set_impl(tbl, key, true, value, hash, update);
}

int tbl_set_str(HashTbl* tbl, const char* key, uint32_t len, const void* value)
{
    return tbl_do_set_str(tbl, key, len, value, false);
}

int tbl_set_sbuf(HashTbl* tbl, StaticBuffer* key, const void* value)
{
    return tbl_set_impl(tbl, (int64_t)key, false, value, hash_str(sbuf_str(key), sbuf_length(key)), false);
}

int tbl_set_int(HashTbl* tbl, int64_t key, const void* value)
{
    return tbl_do_set_int(tbl, key, value, false);
}

int tbl_update_str(HashTbl* tbl, const char* key, uint32_t len, const void* value)
{
    return tbl_do_set_str(tbl, key, len, value, true);
}

int tbl_update_sbuf(HashTbl* tbl, StaticBuffer* key, const void* value)
{
    return tbl_set_impl(tbl, (int64_t)key, false, value, hash_str(sbuf_str(key), sbuf_length(key)), true);
}

int tbl_update_int(HashTbl* tbl, int64_t key, const void* value)
{
    return tbl_do_set_int(tbl, key, value, true);
}

static void* tbl_get_impl(HashTbl* tbl, int64_t key, uint32_t len, int isIntKey, uint32_t hash)
{
    uint32_t capMinusOne = tbl->capacity - 1;
    uint32_t size = tbl->entSize;
    uint32_t pos = hash & capMinusOne;
    byte* data = tbl->data;
    HashTblEnt* ent;
    uint32_t mainPos;
    
    if (!data)
        return NULL;
    
    ent = (HashTblEnt*)&data[pos * size];
    
    if (ent_is_empty(ent))
        return NULL;
    
    mainPos = ent->hash & capMinusOne;
    
    if (mainPos != pos)
        return NULL;
    
    for (;;)
    {
        if (ent->hash == hash && ent_is_int_key(ent) == (uint32_t)isIntKey)
        {
            if (isIntKey)
            {
                if (ent->keyInt == key)
                    return ent->data;
            }
            else
            {
                if (sbuf_length(ent->keyStr) == len && strncmp(sbuf_str(ent->keyStr), (const char*)key, len) == 0)
                    return ent->data;
            }
        }
        
        if (ent_has_next(ent))
            ent = (HashTblEnt*)&tbl->data[ent_get_next(ent) * size];
        else
            break;
    }
    
    return NULL;
}

void* tbl_get_str_raw(HashTbl* tbl, const char* key, uint32_t len)
{
    uint32_t hash;
    
    if (!key) return NULL;
    
    if (len == 0)
        len = strlen(key);

    hash = hash_str(key, len);
    
    return tbl_get_impl(tbl, (int64_t)key, len, false, hash);
}

void* tbl_get_int_raw(HashTbl* tbl, int64_t key)
{
    uint32_t hash = hash_int64(key);
    return tbl_get_impl(tbl, key, 0, true, hash);
}

StaticBuffer* tbl_get_key_str(HashTbl* tbl, const char* key, uint32_t len)
{
    uint32_t hash = hash_str(key, len);
    uint32_t capMinusOne = tbl->capacity - 1;
    uint32_t size = tbl->entSize;
    uint32_t pos = hash & capMinusOne;
    byte* data = tbl->data;
    HashTblEnt* ent;
    uint32_t mainPos;
    
    if (!data)
        return NULL;
    
    ent = (HashTblEnt*)&data[pos * size];
    
    if (ent_is_empty(ent))
        return NULL;
    
    mainPos = ent->hash & capMinusOne;
    
    if (mainPos != pos)
        return NULL;
    
    for (;;)
    {
        if (ent->hash == hash && !ent_is_int_key(ent))
        {
            if (sbuf_length(ent->keyStr) == len && strncmp(sbuf_str(ent->keyStr), key, len) == 0)
                return ent->keyStr;
        }
        
        if (ent_has_next(ent))
            ent = (HashTblEnt*)&tbl->data[ent_get_next(ent) * size];
        else
            break;
    }
    
    return NULL;
}

static int tbl_remove_impl(HashTbl* tbl, int64_t key, uint32_t len, int isIntKey, uint32_t hash)
{
    uint32_t capMinusOne = tbl->capacity - 1;
    uint32_t entSize = tbl->entSize;
    uint32_t freeIndex = tbl->freeIndex;
    uint32_t pos = hash & capMinusOne;
    byte* data = tbl->data;
    HashTblEnt* prev = NULL;
    HashTblEnt* ent;
    uint32_t mainPos;
    
    if (!data)
        return ERR_NotFound;
    
    ent = (HashTblEnt*)&data[entSize * pos];
    
    if (ent_is_empty(ent))
        return ERR_NotFound;
    
    mainPos = ent->hash & capMinusOne;
    
    if (mainPos != pos)
        return ERR_NotFound;
    
    for (;;)
    {
        if (ent->hash == hash && ent_is_int_key(ent) == (uint32_t)isIntKey)
        {
            if (isIntKey)
            {
                if (ent->keyInt != key)
                    continue;
            }
            else
            {
                if (sbuf_length(ent->keyStr) != len || strncmp(sbuf_str(ent->keyStr), (const char*)key, len) != 0)
                    continue;
            }

            /* Found the one we want to remove */
            if (!isIntKey)
                ent->keyStr = sbuf_drop(ent->keyStr);
            
            /* Need to fix links leading to (or following) this, if there were any */
            if (prev)
            {
                /* prev -> cur -> next ==> prev -> next */
                uint32_t cur = ent_get_next(prev);
                ent_set_next(prev, ent_get_next(ent));
                
                if (freeIndex > cur)
                    tbl->freeIndex = cur;
            }
            else
            {
                uint32_t next = ent_get_next(ent);
                
                if (next != NEXT_INVALID)
                {
                    /* [main] -> next ==> [next] */
                    prev = ent;
                    ent = (HashTblEnt*)&tbl->data[next * entSize];
                    memcpy(prev, ent, entSize);
                    
                    if (freeIndex > next)
                        tbl->freeIndex = next;
                }
            }
            
            /* Bit confusing, but the key we are setting to NULL might not be the key we just destroyed; intended */
            ent->keyInt = 0;
            ent->hash = 0;
            ent_set_next(ent, NEXT_INVALID);
            return ERR_None;
        }
        
        /* Not the one we were looking for, move down the chain */
        if (ent_has_next(ent))
        {
            prev = ent;
            ent = (HashTblEnt*)&tbl->data[ent_get_next(ent) * entSize];
        }
        else
        {
            break;
        }
    }
    
    return ERR_NotFound;
}

int tbl_remove_str(HashTbl* tbl, const char* key, uint32_t len)
{
    uint32_t hash;
    
    if (!key) return ERR_Invalid;
    
    if (len == 0)
        len = strlen(key);

    hash = hash_str(key, len);
    
    return tbl_remove_impl(tbl, (int64_t)key, len, false, hash);
}

int tbl_remove_int(HashTbl* tbl, int64_t key)
{
    uint32_t hash = hash_int64(key);
    return tbl_remove_impl(tbl, key, 0, true, hash);
}

void tbl_for_each(HashTbl* tbl, HashTblElemCallback func)
{
    uint32_t entSize = tbl->entSize;
    byte* data = tbl->data;
    uint32_t n = tbl->capacity;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        HashTblEnt* ent = (HashTblEnt*)data;
        
        if (!ent_is_empty(ent))
            func(ent->data);
        
        data += entSize;
    }
}

void tbl_for_each_with_tbl(HashTbl* tbl, HashTblContainerElemCallback func)
{
    uint32_t entSize = tbl->entSize;
    byte* data = tbl->data;
    uint32_t n = tbl->capacity;
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        HashTblEnt* ent = (HashTblEnt*)data;
        
        if (!ent_is_empty(ent))
            func(tbl, ent->data);
        
        data += entSize;
    }
}

#undef ent_is_empty
#undef ent_is_int_key
#undef ent_set_int_key
#undef ent_set_str_key
#undef ent_set_next
#undef ent_get_next
#undef ent_has_next

#undef MIN_CAPACITY
#undef NEXT_INVALID
#undef FREE_INDEX_INVALID
