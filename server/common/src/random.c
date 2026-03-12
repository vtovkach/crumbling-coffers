#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/random.h>
#include <errno.h>
#include <stdio.h>

bool secure_random_bytes(void *target, size_t nbytes)
{
    uint8_t *buf = (uint8_t *)target;
    size_t offset = 0;

    while (offset < nbytes)
    {
        ssize_t n = getrandom(buf + offset, nbytes - offset, 0);

        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }

        offset += (size_t)n;
    }
    
    return true;
}