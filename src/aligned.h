
#ifndef ALIGNED_H
#define ALIGNED_H

#include "define.h"
/*#include "random.h"
#include "eqp_string.h"*/

typedef struct {
    uint32_t    cursor;
    uint32_t    length;
    byte*       buffer;
} Aligned;

void aligned_init(Aligned* a, void* ptr, uint32_t len);
void aligned_init_cursor_at(Aligned* a, void* ptr, uint32_t len, uint32_t cursor);
void aligned_init_copy(Aligned* dst, Aligned* src);
uint32_t aligned_advance_by(Aligned* a, uint32_t len);
#define aligned_advance_by_sizeof(a, type) aligned_advance_by((a), sizeof(type))
uint32_t aligned_reverse_by(Aligned* a, uint32_t len);
bool aligned_check_bounds(Aligned* a, uint32_t len);
/* Returns the number of bytes advanced over, not counting the null terminator, or -1 if there is no null terminator before the end */
int aligned_advance_past_null_terminator(Aligned* a);
#define aligned_size(a) ((a)->length)
#define aligned_remaining_space(a) ((a)->length - (a)->cursor)
#define aligned_remaining_data(a) aligned_remaining_space(a)
#define aligned_position(a) ((a)->cursor)
#define aligned_all(a) ((a)->buffer)
#define aligned_current(a) ((a)->buffer + (a)->cursor)
#define aligned_current_type(a, type) (type*)aligned_current(a)
#define aligned_at_position(a, pos) ((a)->buffer + (pos))

#define aligned_set_length(a, len) ((a)->length = (len))
#define aligned_reduce_size_by(a, by) ((a)->length -= (by))
#define aligned_reset_cursor(a) ((a)->cursor = 0)
#define aligned_reset_cursor_to(a, offset) ((a)->cursor = (offset))

/* Read */
uint8_t aligned_read_uint8(Aligned* a);
#define aligned_read_int8(a) ((int8_t)aligned_read_uint8((a)))
#define aligned_read_byte(a) (aligned_read_uint8((a)))
uint16_t aligned_read_uint16(Aligned* a);
#define aligned_read_int16(a) ((int16_t)aligned_read_uint16((a)))
uint32_t aligned_read_uint32(Aligned* a);
#define aligned_read_int32(a) ((int32_t)aligned_read_uint32((a)))
#define aligned_read_int(a) aligned_read_int32(a)
uint64_t aligned_read_uint64(Aligned* a);
#define aligned_read_int64(a) ((int64_t)aligned_read_uint64((a)))
float aligned_read_float(Aligned* a);
void aligned_read_buffer(Aligned* a, void* dst, uint32_t len);

uint8_t aligned_peek_uint8(Aligned* a);
#define aligned_peek_int8(a) ((int8_t)aligned_peek_uint8((a)))
#define aligned_peek_byte(a) (aligned_peek_uint8((a)))

/* Write */
#define aligned_write_zero_all(a) (memset((a)->buffer, 0, (a)->length))
#define aligned_write_memset_no_advance(a, val, bytes) (memset(aligned_current((a)), (val), (bytes)))
void aligned_write_memset(Aligned* a, int val, uint32_t len);
void aligned_write_uint8(Aligned* a, uint8_t v);
#define aligned_write_byte(a, v) aligned_write_uint8((a), (v))
#define aligned_write_int8(a, v) (aligned_write_uint8((a), (uint8_t)(v)))
#define aligned_write_bool(a, v) (aligned_write_uint8((a), (v) ? 1 : 0))
void aligned_write_uint16(Aligned* a, uint16_t v);
#define aligned_write_int16(a, v) (aligned_write_uint16((a), (uint16_t)(v)))
void aligned_write_uint32(Aligned* a, uint32_t v);
#define aligned_write_int32(a, v) (aligned_write_uint32((a), (uint32_t)(v)))
#define aligned_write_int(a, v) aligned_write_int32(a, v)
void aligned_write_uint64(Aligned* a, uint64_t v);
#define aligned_write_int64(a, v) (aligned_write_uint64((a), (uint64_t)(v)))
void aligned_write_float(Aligned* a, float v);
void aligned_write_buffer(Aligned* a, const void* data, uint32_t len);
/*#define aligned_write_string_null_terminated(a, str) aligned_write_buffer((a), string_data(str), (string_length(str) + 1))*/
#define aligned_write_literal_null_terminated(a, str) aligned_write_buffer((a), str, sizeof(str))
void aligned_write_random(Aligned* a, int bytes);
void aligned_write_zeroes(Aligned* a, uint32_t count);
/* Always advances n characters, regardless of how many are actually written */
void aligned_write_snprintf_and_advance_by(Aligned* a, uint32_t n, const char* fmt, ...);

void aligned_write_reverse_uint8(Aligned* a, uint8_t v);
void aligned_write_reverse_uint16(Aligned* a, uint16_t v);

#endif/*ALIGNED_H*/
