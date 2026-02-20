#ifndef _UTIL_H
#define _UTIL_H

#define TIME_BUFFER_SIZE 64

int redirect_stderr(const char *path);

void getTime(char *dest, size_t buf_size);

#endif