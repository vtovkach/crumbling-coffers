#include <stdlib.h>
#include <string.h>

#include "broker.h"
#include "broker-config.h"
#include "ds/spsc_queue.h"

struct DataFrame 
{
    size_t  size;
    void   *data;
};

struct Broker 
{
    SPSCQueue *q_orch;        // orchestrator → session manager
    SPSCQueue *q_session_man; // session manager → orchestrator
};

struct Broker *init_broker(void)
{
    struct Broker *broker = calloc(1, sizeof(struct Broker));
    if (!broker) return NULL;

    broker->q_orch = spscq_create(
        BROKER_QUEUE_CAPACITY, sizeof(struct DataFrame));
    if (!broker->q_orch)
    {
        free(broker);
        return NULL;
    }

    broker->q_session_man = spscq_create(
        BROKER_QUEUE_CAPACITY, sizeof(struct DataFrame));
    if (!broker->q_session_man)
    {
        spscq_destroy(broker->q_orch);
        free(broker);
        return NULL;
    }

    return broker;
}

void destroy_broker(struct Broker *broker)
{
    if (!broker) return;
    spscq_destroy(broker->q_orch);
    spscq_destroy(broker->q_session_man);
    free(broker);
}

static int push_frame(SPSCQueue *q, void *data, size_t size)
{
    void *copy = malloc(size);
    if (!copy) return -1;

    memcpy(copy, data, size);

    struct DataFrame frame = { .size = size, .data = copy };
    if (spscq_push(q, &frame) != 0)
    {
        free(copy);
        return -1;
    }

    return 0;
}

static int pop_frame(SPSCQueue *q, void *dest, size_t dest_s)
{
    struct DataFrame frame;
    if (spscq_pop(q, &frame) != 0) return -1;

    if (dest_s < frame.size)
    {
        free(frame.data);
        return -1;
    }

    memcpy(dest, frame.data, frame.size);
    free(frame.data);
    return 0;
}

int push_data_orch(struct Broker *broker, void *data, size_t size)
{
    return push_frame(broker->q_orch, data, size);
}

int push_data_sessions_man(struct Broker *broker, void *data, size_t size)
{
    return push_frame(broker->q_session_man, data, size);
}

int pop_data_orch(struct Broker *broker, void *dest, size_t dest_s)
{
    return pop_frame(broker->q_orch, dest, dest_s);
}

int pop_data_sessions_man(struct Broker *broker, void *dest, size_t dest_s)
{
    return pop_frame(broker->q_session_man, dest, dest_s);
}
