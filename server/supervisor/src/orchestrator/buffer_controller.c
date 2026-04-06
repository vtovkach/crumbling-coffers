#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "orchestrator/buffer_controller.h"
#include "ds/hashmap.h"
#include "server-config.h"

struct Buffer
{
    void   *input;
    size_t  input_capacity;
    size_t  input_size;

    void   *output;
    size_t  output_capacity;
    size_t  output_size;
};

struct BufferController
{
    HashTable    *buffers;
    unsigned int  num_active;
};

static unsigned int hash(const void *key, unsigned int table_size)
{
    uint32_t x;
    memcpy(&x, key, sizeof(uint32_t));

    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;

    return x & (table_size - 1);
}

struct BufferController *bc_init(void)
{
    struct BufferController *bc = malloc(sizeof(*bc));
    if (!bc) return NULL;

    bc->buffers = ht_create(sizeof(int), 1, sizeof(struct Buffer), 1,
                            hash, HASH_TABLE_SIZE);
    if (!bc->buffers)
    {
        free(bc);
        return NULL;
    }

    bc->num_active = 0;
    return bc;
}

void bc_destroy(struct BufferController *bc)
{
    if (!bc) return;

    for (unsigned int i = 0; i < bc->buffers->hash_table_size; i++)
    {
        Node *cur = bc->buffers->hash_table[i];
        while (cur)
        {
            struct Buffer *buf = (struct Buffer *)cur->value;
            if (buf)
            {
                free(buf->input);
                free(buf->output);
            }
            cur = cur->nextNode;
        }
    }

    ht_destroy(bc->buffers);
    free(bc);
}

int bc_add(struct BufferController *bc, int fd)
{
    struct Buffer buf = {0};

    buf.input = malloc(TCP_SEGMENT_SIZE);
    if (!buf.input) return -1;

    buf.output = malloc(TCP_SEGMENT_SIZE);
    if (!buf.output)
    {
        free(buf.input);
        return -1;
    }

    buf.input_capacity  = TCP_SEGMENT_SIZE;
    buf.input_size      = 0;
    buf.output_capacity = TCP_SEGMENT_SIZE;
    buf.output_size     = 0;

    if (ht__insert_internal(bc->buffers, &fd, &buf) < 0)
    {
        free(buf.input);
        free(buf.output);
        return -1;
    }

    bc->num_active++;
    return 0;
}

void bc_remove(struct BufferController *bc, int fd)
{
    struct Buffer *buf = ht__get_internal(bc->buffers, &fd, sizeof(int));
    if (!buf) return;

    free(buf->input);
    free(buf->output);

    ht__remove_internal(bc->buffers, &fd);
    bc->num_active--;
}

bool bc_is_output_free(const struct BufferController *bc, int fd)
{
    struct Buffer *buf = ht__get_internal(bc->buffers, &fd, sizeof(int));
    if (!buf) return false;
    return buf->output_size == 0;
}

bool bc_is_input_full(const struct BufferController *bc, int fd)
{
    struct Buffer *buf = ht__get_internal(bc->buffers, &fd, sizeof(int));
    if (!buf) return false;
    return buf->input_size == buf->input_capacity;
}

size_t bc_input_available_space(const struct BufferController *bc, int fd)
{
    struct Buffer *buf = ht__get_internal(bc->buffers, &fd, sizeof(int));
    if (!buf) return 0;
    return buf->input_capacity - buf->input_size;
}

int bc_push_input(struct BufferController *bc, int fd, const void *data, size_t size)
{
    struct Buffer *buf = ht__get_internal(bc->buffers, &fd, sizeof(int));
    if (!buf) return -1;
    if (buf->input_capacity - buf->input_size < size) return -2;

    memcpy((uint8_t *)buf->input + buf->input_size, data, size);
    buf->input_size += size;

    return 0;
}

int bc_copy_input(struct BufferController *bc, int fd, void *dest, size_t dest_size)
{
    struct Buffer *buf = ht__get_internal(bc->buffers, &fd, sizeof(int));
    if (!buf) return -1;

    size_t to_copy = buf->input_size < dest_size ? buf->input_size : dest_size;
    memcpy(dest, buf->input, to_copy);
    buf->input_size = 0;

    return (int)to_copy;
}

int bc_push_output(struct BufferController *bc, int fd, const void *data, size_t size)
{
    struct Buffer *buf = ht__get_internal(bc->buffers, &fd, sizeof(int));
    if (!buf) return -1;
    if (buf->output_capacity - buf->output_size < size) return -2;

    memcpy((uint8_t *)buf->output + buf->output_size, data, size);
    buf->output_size += size;

    return 0;
}

int bc_copy_output(const struct BufferController *bc, int fd, void *dest, size_t dest_size)
{
    struct Buffer *buf = ht__get_internal(bc->buffers, &fd, sizeof(int));
    if (!buf) return -1;

    size_t to_copy = buf->output_size < dest_size ? buf->output_size : dest_size;
    memcpy(dest, buf->output, to_copy);

    return (int)to_copy;
}

int bc_update_output(struct BufferController *bc, int fd, size_t bytes_sent)
{
    struct Buffer *buf = ht__get_internal(bc->buffers, &fd, sizeof(int));
    if (!buf) return -1;

    if (bytes_sent >= buf->output_size)
    {
        buf->output_size = 0;
        return 0;
    }

    memmove(buf->output,
            (uint8_t *)buf->output + bytes_sent,
            buf->output_size - bytes_sent);
    buf->output_size -= bytes_sent;

    return 0;
}
