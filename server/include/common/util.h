#ifndef _UTIL_H
#define _UTIL_H

#define TIME_BUFFER_SIZE 64

int redirect_stderr(const char *path);

void getTime(char *dest, size_t buf_size);

void log_error_fd(FILE *const log_file, const char *err_msg, int conn_fd, int errno_code);

void log_error(FILE *const log_file, const char *err_msg, int errno_code);

#endif