#ifndef _HERALD_
#define _HERALD_

#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "server-config.h"

struct Herald;

struct Herald *herald_init();

void herald_destroy(struct Herald *herald);

bool herald_is_ready(const struct Herald *herald);

int herald_write(struct Herald *herald, void *packet, size_t size);

int herald_read(struct Herald *herald, void *dest, size_t dest_size);

#endif 