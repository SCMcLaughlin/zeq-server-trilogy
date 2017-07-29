
#include "ringbuf.h"
#include "util_alloc.h"
#include "util_atomic.h"
#include "util_clock.h"
#include "util_semaphore.h"

#ifdef PLATFORM_WINDOWS
/* data[0]: "nonstandard extension used: zero-sized array in struct/union" */
# pragma warning(disable: 4200)
#endif

typedef struct {
    int16_t     nextIndex;
    atomic8_t   hasBeenRead;
    atomic8_t   hasBeenWritten;
    int         opcode;
    byte        data[0];
} RingBlock;

struct RingBuf {
    atomic16_t  readStart;
    atomic16_t  readEnd;
    atomic16_t  writeStart;
    atomic16_t  writeEnd;
    uint32_t    dataSize;
    Semaphore   semaphore;
    byte        data[0];
};

#define ringbuf_get_block(rb, n) ((RingBlock*)(&((rb)->data[(sizeof(RingBlock) + (rb)->dataSize) * (n)])))

static int ringbuf_init(RingBuf* rb, uint32_t dataSize, uint32_t slotCount)
{
    uint32_t i;
    uint32_t n = slotCount - 1;
    RingBlock* block;

    rb->dataSize = dataSize;

    block = ringbuf_get_block(rb, 0);
    block->nextIndex = 1;
    atomic8_set(&block->hasBeenRead, 0);
    atomic8_set(&block->hasBeenWritten, 0);

    for (i = 1; i < n; i++)
    {
        block = ringbuf_get_block(rb, i);
        block->nextIndex = i + 1;
        atomic8_set(&block->hasBeenRead, 0);
        atomic8_set(&block->hasBeenWritten, 0);
    }

    block = ringbuf_get_block(rb, n);
    block->nextIndex = 0;
    atomic8_set(&block->hasBeenRead, 0);
    atomic8_set(&block->hasBeenWritten, 0);

    atomic16_set(&rb->readStart, 0);
    atomic16_set(&rb->readEnd, 0);
    atomic16_set(&rb->writeStart, 0);
    atomic16_set(&rb->writeEnd, 0);
    
    return semaphore_init(&rb->semaphore);
}

RingBuf* ringbuf_create(uint32_t dataSize, uint32_t slotCount)
{
    RingBuf* rb;

    assert(slotCount >= RINGBUF_MIN_PACKETS && slotCount <= RINGBUF_MAX_PACKETS);

    rb = alloc_bytes_type(sizeof(RingBuf) + (dataSize + sizeof(RingBlock)) * slotCount, RingBuf);
    
    if (!rb)
    {
        return NULL;
    }
    
    if (ringbuf_init(rb, dataSize, slotCount))
    {
        free(rb);
        return NULL;
    }
    
    return rb;
}

RingBuf* ringbuf_destroy(RingBuf* rb)
{
    if (rb)
    {
        semaphore_deinit(&rb->semaphore);
        free(rb);
    }

    return NULL;
}

int ringbuf_wait(RingBuf* rb)
{
    return semaphore_wait(&rb->semaphore);
}

int ringbuf_try_wait(RingBuf* rb)
{
    return semaphore_try_wait(&rb->semaphore);
}

int ringbuf_signal(RingBuf* rb)
{
    return semaphore_trigger(&rb->semaphore);
}

static void ringbuf_push_impl(RingBuf* rb, RingBlock* block, int opcode, const void* p)
{
    block->opcode = opcode;

    if (p)
    {
        memcpy(&block->data[0], p, rb->dataSize);
    }

    atomic8_set(&block->hasBeenWritten, 1);
    
    /* Advance over completely written blocks, as far as possible */
    for (;;)
    {
        int16_t index = atomic16_get(&rb->writeStart);
        block = ringbuf_get_block(rb, index);
        
        /*
            If the hasBeenWritten flag has already been cleared, another thread must have already advanced for us
            Or, we've reached the end of what was available
        */
        if (!atomic8_cmp_xchg_strong(&block->hasBeenWritten, 1, 0))
        {
            break;
        }
        
        /* Advance writeStart and loop */
        atomic16_cmp_xchg_strong(&rb->writeStart, index, block->nextIndex);
    }
}

int ringbuf_push(RingBuf* rb, int opcode, const void* p)
{
    for (;;)
    {
        int16_t index = atomic16_get(&rb->writeEnd);
        RingBlock* block = ringbuf_get_block(rb, index);
        
        if (block->nextIndex == atomic16_get(&rb->readStart))
        {
            return ERR_OutOfSpace;
        }
        
        if (atomic16_cmp_xchg_weak(&rb->writeEnd, index, block->nextIndex))
        {
            ringbuf_push_impl(rb, block, opcode, p);
            return ringbuf_signal(rb);
        }
    }
}

static void ringbuf_pop_impl(RingBuf* rb, RingBlock* block, int* opcode, void* p)
{
    if (opcode)
    {
        *opcode = block->opcode;
    }

    if (p)
    {
        memcpy(p, &block->data[0], rb->dataSize);
    }

    atomic8_set(&block->hasBeenRead, 1);
    
    /* Advance over completely read blocks, as far as possible */
    for (;;)
    {
        int16_t index = atomic16_get(&rb->readStart);
        block = ringbuf_get_block(rb, index);
        
        /*
            If the hasBeenRead flag has already been cleared, another thread must have already advanced for us
            Or, we've reached the end of what was available
        */
        if (!atomic8_cmp_xchg_strong(&block->hasBeenRead, 1, 0))
        {
            return;
        }
        
        /* Advance readStart and loop */
        atomic16_cmp_xchg_strong(&rb->readStart, index, block->nextIndex);
    }
}

int ringbuf_pop(RingBuf* rb, int* opcode, void* p)
{
    for (;;)
    {
        int16_t index = atomic16_get(&rb->readEnd);
        RingBlock* block = ringbuf_get_block(rb, index);
        
        if (index == atomic16_get(&rb->writeStart))
        {
            return ERR_Again;
        }
        
        if (atomic16_cmp_xchg_weak(&rb->readEnd, index, block->nextIndex))
        {
            ringbuf_pop_impl(rb, block, opcode, p);
            return ERR_None;
        }
    }
}

int ringbuf_wait_for_packet(RingBuf* rb, int* opcode, void* p)
{
    for (;;)
    {
        int rc = ringbuf_wait(rb);

        if (rc) return rc;
        
        rc = ringbuf_pop(rb, opcode, p);
        
        if (rc != ERR_Again) return rc;
    }
}

int ringbuf_wait_for_packet_with_timeout(RingBuf* rb, uint32_t timeoutMs, int* opcode, void* data)
{
    uint64_t timestamp = clock_milliseconds() + (uint64_t)timeoutMs;
    int rc;

    for (;;)
    {
        rc = semaphore_try_wait(&rb->semaphore);

        if (ERR_None == rc)
        {
            rc = ringbuf_pop(rb, opcode, data);

            if (rc != ERR_Again) return rc;
        }

        if (clock_milliseconds() >= timestamp)
            break;
    }

    return rc;
}
