
#ifndef RINGBUF_H
#define RINGBUF_H

#include "define.h"

#define RINGBUF_MIN_PACKETS 2
#define RINGBUF_MAX_PACKETS SHRT_MAX

typedef struct RingBuf RingBuf;

RingBuf* ringbuf_create(uint32_t dataSize, uint32_t slotCount);
#define ringbuf_create_type(type, slots) (ringbuf_create(sizeof(type), (slots)))
#define ringbuf_create_no_content(slots) (ringbuf_create(0, (slots)))
RingBuf* ringbuf_destroy(RingBuf* rb);
#define ringbuf_destroy_if_exists(rb) do { if ((rb)) { (rb) = ringbuf_destroy((rb)); } } while(0)

int ringbuf_wait(RingBuf* rb);
int ringbuf_try_wait(RingBuf* rb);
int ringbuf_signal(RingBuf* rb);

int ringbuf_push(RingBuf* rb, int opcode, const void* data);
int ringbuf_pop(RingBuf* rb, int* opcode, void* data);
int ringbuf_wait_for_packet(RingBuf* rb, int* opcode, void* data);
/* Only intended to be used when shutting down threads */
int ringbuf_wait_for_packet_with_timeout(RingBuf* rb, uint32_t timeoutMs, int* opcode, void* data);

#endif/*RINGBUF_H*/
