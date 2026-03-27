#include "spsc_queue.h"

#include <stdlib.h>
#include <string.h>

static size_t spscq_next_index(size_t index, size_t capacity)
{
    index++;
    if (index == capacity)
        index = 0;
    return index;
}

SPSCQueue *spscq_create(size_t capacity, size_t elem_size)
{
    SPSCQueue *q;

    if (capacity < 2 || elem_size == 0)
        return NULL;

    q = calloc(1, sizeof(*q));
    if (!q)
        return NULL;

    q->buffer = calloc(capacity, elem_size);
    if (!q->buffer)
    {
        free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->elem_size = elem_size;
    atomic_init(&q->head, 0);
    atomic_init(&q->tail, 0);

    return q;
}

void spscq_destroy(SPSCQueue *q)
{
    if (!q)
        return;

    free(q->buffer);
    free(q);
}

int spscq_push(SPSCQueue *q, const void *elem)
{
    size_t tail;
    size_t next_tail;
    size_t head;
    uint8_t *slot;

    if (!q || !elem)
        return -1;

    tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    next_tail = spscq_next_index(tail, q->capacity);

    /*
     * Acquire pairs with consumer's release-store to head.
     * Producer must see latest head before deciding queue is full.
     */
    head = atomic_load_explicit(&q->head, memory_order_acquire);

    if (next_tail == head)
        return -1; /* full */

    slot = q->buffer + (tail * q->elem_size);
    memcpy(slot, elem, q->elem_size);

    /*
     * Release publishes the written element before tail is advanced.
     */
    atomic_store_explicit(&q->tail, next_tail, memory_order_release);

    return 0;
}

int spscq_pop(SPSCQueue *q, void *out_elem)
{
    size_t head;
    size_t tail;
    size_t next_head;
    uint8_t *slot;

    if (!q || !out_elem)
        return -1;

    head = atomic_load_explicit(&q->head, memory_order_relaxed);

    /*
     * Acquire pairs with producer's release-store to tail.
     * Consumer must see element contents after seeing updated tail.
     */
    tail = atomic_load_explicit(&q->tail, memory_order_acquire);

    if (head == tail)
        return -1; /* empty */

    slot = q->buffer + (head * q->elem_size);
    memcpy(out_elem, slot, q->elem_size);

    next_head = spscq_next_index(head, q->capacity);

    /*
     * Release publishes head advancement before producer observes it.
     */
    atomic_store_explicit(&q->head, next_head, memory_order_release);

    return 0;
}

int spscq_is_empty(SPSCQueue *q)
{
    size_t head;
    size_t tail;

    if (!q)
        return 1;

    head = atomic_load_explicit(&q->head, memory_order_acquire);
    tail = atomic_load_explicit(&q->tail, memory_order_acquire);

    return head == tail;
}

int spscq_is_full(SPSCQueue *q)
{
    size_t head;
    size_t tail;
    size_t next_tail;

    if (!q)
        return 0;

    tail = atomic_load_explicit(&q->tail, memory_order_acquire);
    head = atomic_load_explicit(&q->head, memory_order_acquire);
    next_tail = spscq_next_index(tail, q->capacity);

    return next_tail == head;
}

size_t spscq_usable_capacity(SPSCQueue *q)
{
    if (!q || q->capacity == 0)
        return 0;

    return q->capacity - 1;
}