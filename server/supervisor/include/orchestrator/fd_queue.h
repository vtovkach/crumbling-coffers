#ifndef FD_QUEUE_H
#define FD_QUEUE_H

struct FdQueue;

struct FdQueue *fdq_init(void);
void            fdq_destroy(struct FdQueue *q);

void fdq_push(struct FdQueue *q, int fd);
int  fdq_pop(struct FdQueue *q);

#endif
