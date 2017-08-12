
#include "buffer.h"
#include "bit.h"
#include "util_atomic.h"
#include "util_alloc.h"

struct StaticBuffer {
    atomic32_t  refCount;
    uint32_t    len;
};

void dbuf_init(DynamicBuffer* buf)
{
    buf->capacity = 0;
    buf->length = sizeof(StaticBuffer);
    buf->data = NULL;
}

void dbuf_deinit(DynamicBuffer* buf)
{
    buf->capacity = 0;
    buf->length = sizeof(StaticBuffer);

    if (buf->data)
    {
        free(buf->data);
        buf->data = NULL;
    }
}

static int dbuf_realloc(DynamicBuffer* buf, uint32_t len)
{
    uint32_t cap = bit_pow2_greater_or_equal_u32(len);
    byte* ptr = realloc_bytes(buf->data, cap);

    if (!ptr)
    {
        return false;
    }

    buf->capacity = cap;
    buf->data = ptr;
    return true;
}

int dbuf_append(DynamicBuffer* buf, const void* data, uint32_t len)
{
    uint32_t total = buf->length + len + 1;
    uint32_t cap = buf->capacity;
    byte* ptr;

    if (total > cap && !dbuf_realloc(buf, total))
    {
        return ERR_OutOfMemory;
    }

    ptr = buf->data + buf->length;
    buf->length = total;

    memcpy(ptr, data, len);
    ptr[len] = '\0';
    return ERR_None;
}

int dbuf_append_format(DynamicBuffer* buf, const char* fmt, ...)
{
    char cbuf[4096];
    va_list args;
    int len;

    va_start(args, fmt);
    len = vsnprintf(cbuf, sizeof(cbuf), fmt, args);
    va_end(args);

    if (len < 0 || len >= (int)sizeof(cbuf))
    {
        return ERR_Generic;
    }

    return dbuf_append(buf, cbuf, (uint32_t)len);
}

const void* dbuf_data(DynamicBuffer* buf)
{
    return buf->data;
}

uint32_t dbuf_length(DynamicBuffer* buf)
{
    return buf->length - sizeof(StaticBuffer);
}

static byte* sbuf_data_writable(StaticBuffer* buf)
{
    return ((byte*)buf) + sizeof(StaticBuffer);
}

StaticBuffer* dbuf_convert_to_sbuf(DynamicBuffer* dbuf)
{
    StaticBuffer* sbuf = (StaticBuffer*)dbuf->data;

    atomic32_set(&sbuf->refCount, 0);
    sbuf->len = dbuf->length - sizeof(StaticBuffer);

    dbuf->capacity = 0;
    dbuf->length = sizeof(StaticBuffer);
    dbuf->data = NULL;

    return sbuf;
}

StaticBuffer* dbuf_copy_to_sbuf(DynamicBuffer* dbuf)
{
    StaticBuffer* sbuf;
    uint32_t cap = dbuf->capacity;
    uint32_t len = dbuf->length - sizeof(StaticBuffer) + 1;
    byte* ptr;

    if (dbuf->capacity == 0)
    {
        cap = sizeof(StaticBuffer);
    }
    
    sbuf = alloc_bytes_type(cap, StaticBuffer);

    if (!sbuf)
    {
        return NULL;
    }

    atomic32_set(&sbuf->refCount, 0);
    sbuf->len = len;
    ptr = sbuf_data_writable(sbuf);

    if (dbuf->data)
    {
        memcpy(ptr, dbuf->data, len);
    }
    ptr[len] = '\0';

    return sbuf;
}

StaticBuffer* sbuf_create(const void* data, uint32_t len)
{
    uint32_t cap = bit_pow2_greater_or_equal_u32(sizeof(StaticBuffer) + len + 1);
    StaticBuffer* buf = alloc_bytes_type(cap, StaticBuffer);
    byte* ptr;

    if (!buf) return NULL;

    atomic32_set(&buf->refCount, 0);
    buf->len = len;
    ptr = sbuf_data_writable(buf);

    if (data && len)
    {
        memcpy(ptr, data, len);
    }
    ptr[len] = '\0';

    return buf;
}

StaticBuffer* sbuf_create_from_file(const char* path)
{
    FILE* fp = fopen(path, "rb");
    StaticBuffer* sbuf;
    
    if (!fp) return NULL;
    
    sbuf = sbuf_create_from_file_ptr(fp);
    
    fclose(fp);
    return sbuf;
}

StaticBuffer* sbuf_create_from_file_ptr(FILE* fp)
{
    uint32_t cap;
    uint32_t buflen;
    StaticBuffer* buf;
    byte* ptr;
    long filelen;
    
    if (fseek(fp, 0, SEEK_END))
        return NULL;
    
    filelen = ftell(fp);
    
    if (filelen < 0 || fseek(fp, 0, SEEK_SET))
        return NULL;
    
    buflen = (uint32_t)filelen;
    cap = bit_pow2_greater_or_equal_u32(sizeof(StaticBuffer) + buflen + 1);
    buf = alloc_bytes_type(cap, StaticBuffer);
    
    if (!buf) return NULL;
    
    atomic32_set(&buf->refCount, 0);
    buf->len = buflen;
    ptr = sbuf_data_writable(buf);
    
    if (fread(ptr, sizeof(byte), (size_t)buflen, fp) != (size_t)buflen)
    {
        sbuf_drop(buf);
        return NULL;
    }
    
    ptr[buflen] = '\0';
    
    return buf;
}

void sbuf_grab(StaticBuffer* buf)
{
    if (buf)
    {
        atomic32_add(&buf->refCount, 1);
    }
}

StaticBuffer* sbuf_drop(StaticBuffer* buf)
{
    if (buf && atomic32_sub(&buf->refCount, 1) <= 1)
    {
        free(buf);
    }

    return NULL;
}

const void* sbuf_data(StaticBuffer* buf)
{
    return sbuf_data_writable(buf);
}

uint32_t sbuf_length(StaticBuffer* buf)
{
    return buf->len;
}

void sbuf_zero_fill(StaticBuffer* buf)
{
    if (buf)
    {
        memset(sbuf_data_writable(buf), 0, buf->len);
    }
}

const char* sbuf_str_or_empty_string(StaticBuffer* buf)
{
    const char* ret = "";
    
    if (buf)
        ret = sbuf_str(buf);
    
    return ret;
}
