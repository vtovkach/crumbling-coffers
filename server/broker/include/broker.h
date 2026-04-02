#ifndef _BROKER_
#define _BROKER_

#include <stddef.h>

struct Broker;
struct DataFrame;

struct Broker *init_broker(void);
void destroy_broker(struct Broker *broker);

int push_data_orch(struct Broker *broker, void *data, size_t size);
int push_data_sessions_man(struct Broker *broker, void *data, size_t size);

int pop_data_orch(struct Broker *broker, void *dest, size_t dest_s);
int pop_data_sessions_man(struct Broker *broker, void *dest, size_t dest_s);

#endif
