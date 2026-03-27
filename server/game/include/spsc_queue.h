#ifndef SPSC_QUEUE_H
#define SPSC_QUEUE_H

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

typedef struct SPSCQueue
{
    uint8_t *buffer;
    size_t capacity;
    size_t elem_size;

    _Atomic size_t head;
    _Atomic size_t tail;
} SPSCQueue;

/*
 * Creates a new SPSC queue.
 *
 * capacity:
 *   Number of elements allocated in the ring.
 *   Internally, one slot is left unused to distinguish full vs empty.
 *   So usable capacity is (capacity - 1).
 *
 * elem_size:
 *   Size in bytes of one stored element.
 *
 * Returns:
 *   Pointer to initialized queue on success, NULL on failure.
 */
SPSCQueue *spscq_create(size_t capacity, size_t elem_size);

/*
 * Destroys the queue and frees all owned memory.
 */
void spscq_destroy(SPSCQueue *q);

/*
 * Push one element into the queue.
 *
 * elem:
 *   Pointer to the element to copy into the queue.
 *
 * Returns:
 *   0  on success
 *  -1  if queue is full or input invalid
 *
 * Threading:
 *   Must be called only by the single producer thread.
 */
int spscq_push(SPSCQueue *q, const void *elem);

/*
 * Pop one element from the queue.
 *
 * out_elem:
 *   Destination buffer where the element will be copied.
 *
 * Returns:
 *   0  on success
 *  -1  if queue is empty or input invalid
 *
 * Threading:
 *   Must be called only by the single consumer thread.
 */
int spscq_pop(SPSCQueue *q, void *out_elem);

/*
 * Returns 1 if empty, 0 otherwise.
 *
 * Safe for observation, but mainly useful for debugging or light checks.
 */
int spscq_is_empty(SPSCQueue *q);

/*
 * Returns 1 if full, 0 otherwise.
 *
 * Safe for observation, but mainly useful for debugging or light checks.
 */
int spscq_is_full(SPSCQueue *q);

/*
 * Returns usable capacity: total slots minus one reserved slot.
 */
size_t spscq_usable_capacity(SPSCQueue *q);

#endif