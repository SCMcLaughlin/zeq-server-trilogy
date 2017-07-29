
#ifndef BUFFER_H
#define BUFFER_H

#include "define.h"

typedef struct StaticBuffer StaticBuffer;

typedef struct {
    uint32_t    capacity;
    uint32_t    length;
    byte*       data;
} DynamicBuffer;

void dbuf_init(DynamicBuffer* buf);
void dbuf_deinit(DynamicBuffer* buf);

int dbuf_append(DynamicBuffer* buf, const void* data, uint32_t len);
int dbuf_append_format(DynamicBuffer* buf, const char* fmt, ...);

const void* dbuf_data(DynamicBuffer* buf);
uint32_t dbuf_length(DynamicBuffer* buf);
StaticBuffer* dbuf_convert_to_sbuf(DynamicBuffer* buf);
StaticBuffer* dbuf_copy_to_sbuf(DynamicBuffer* buf);

#define dbuf_data_type(buf, type) ((type*)buf_data((buf)))
#define dbuf_str(buf) buf_data_type(buf, const char)

#define dbuf_empty(buf) (dbuf_length((buf)) == 0)


StaticBuffer* sbuf_create(const void* data, uint32_t len);
StaticBuffer* sbuf_create_from_file(const char* path);
StaticBuffer* sbuf_create_from_file_ptr(FILE* fp);
#define sbuf_create_from_literal(str) sbuf_create(str, sizeof(str) - 1)

void sbuf_grab(StaticBuffer* buf);
StaticBuffer* sbuf_drop(StaticBuffer* buf);

const void* sbuf_data(StaticBuffer* buf);
uint32_t sbuf_length(StaticBuffer* buf);

void sbuf_zero_fill(StaticBuffer* buf);

#define sbuf_data_type(buf, type) ((type*)sbuf_data((buf)))
#define sbuf_str(buf) (sbuf_data_type(buf, const char))

#endif/*BUFFER_H*/
