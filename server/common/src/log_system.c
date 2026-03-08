#include <stdio.h>
#include <string.h>

#include "util.h"

void log_message(FILE *const log_file, const char *msg)
{
    char time[TIME_BUFFER_SIZE];
    getTime(time, TIME_BUFFER_SIZE);

    fprintf(log_file, "%s %s", time, msg);
}

void log_error(FILE *const log_file, const char *msg, int errno_code)
{
    char time[TIME_BUFFER_SIZE];
    getTime(time, TIME_BUFFER_SIZE);

    if(errno_code == 0)
        fprintf(log_file, "%s %s\n", time, msg);
    else
        fprintf(log_file, "%s %s: %s\n", time, msg, strerror(errno_code));
}

void log_error_fd(FILE *const log_file, const char *err_msg, int conn_fd, int errno_code)
{
    char time[TIME_BUFFER_SIZE]; 
    getTime(time, TIME_BUFFER_SIZE);

    if(errno_code == 0)
        fprintf(log_file, "%s %s fd (%d)\n", time, err_msg, conn_fd);
    else
        fprintf(log_file, "%s %s fd (%d): %s\n", time, err_msg, conn_fd, strerror(errno_code));
}