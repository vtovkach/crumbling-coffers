#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "util.h"
#include "server-config.h"

void log_message(FILE *const log_file, const char *msg)
{
    char time[TIME_BUFFER_SIZE];
    getTime(time, TIME_BUFFER_SIZE);

    fprintf(log_file, "%s %s", time, msg);
}

void log_net_data(FILE *const log_file, char *const buf, size_t buf_size)
{
    char time[TIME_BUFFER_SIZE];
    getTime(time, TIME_BUFFER_SIZE);

    fprintf(log_file, "%s Received Data: ", time);

    for(size_t i = 0; i < buf_size; i++)
    {
        if(buf[i] == '\0') 
        {
            fputc('*', log_file);
            continue;
        }
        fputc(buf[i], log_file);
    }
    fputc('\n', log_file);

    // Remove fflush later can hurt performance if called often 
    fflush(log_file);
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