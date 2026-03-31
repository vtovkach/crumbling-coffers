#ifndef _RANDOM_H
#define _RANDOM_H

#include <stdbool.h>
#include <stddef.h>

bool secure_random_bytes(void *target, size_t nbytes);

#endif