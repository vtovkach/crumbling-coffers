#ifndef BUFFER_CONTROLLER_H
#define BUFFER_CONTROLLER_H

#include <stddef.h>
#include <stdbool.h>

struct BufferController;

struct BufferController *bc_init(void);
void bc_destroy(struct BufferController *bc);

int  bc_add(struct BufferController *bc, int fd);
void bc_remove(struct BufferController *bc, int fd);

bool   bc_is_input_full(const struct BufferController *bc, int fd);
size_t bc_input_available_space(const struct BufferController *bc, int fd);

int bc_push_input(struct BufferController *bc, int fd, const void *data, size_t size);
int bc_copy_input(struct BufferController *bc, int fd, void *dest, size_t dest_size);

bool bc_is_output_free(const struct BufferController *bc, int fd);
int  bc_push_output(struct BufferController *bc, int fd, const void *data, size_t size);
int  bc_copy_output(const struct BufferController *bc, int fd, void *dest, size_t dest_size);
int  bc_update_output(struct BufferController *bc, int fd, size_t bytes_sent);

#endif
