#include <stdlib.h>

#include "orchestrator/fd_queue.h"
#include "server-config.h"

#define FD_QUEUE_CAPACITY EPOLL_MAX_EVENTS

struct FdQueue {
    int fds[FD_QUEUE_CAPACITY];
    int head;
    int count;
};

struct FdQueue *fdq_init(void)
{
    struct FdQueue *q = calloc(1, sizeof(*q));
    return q;
}

void fdq_destroy(struct FdQueue *q)
{
    free(q);
}

void fdq_push(struct FdQueue *q, int fd)
{
    if(q->count >= FD_QUEUE_CAPACITY) return;
    int tail = (q->head + q->count) % FD_QUEUE_CAPACITY;
    q->fds[tail] = fd;
    q->count++;
}

int fdq_pop(struct FdQueue *q)
{
    if(q->count == 0) return -1;
    int fd = q->fds[q->head];
    q->head = (q->head + 1) % FD_QUEUE_CAPACITY;
    q->count--;
    return fd;
}
