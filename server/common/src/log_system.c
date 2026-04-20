#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <pthread.h>

#include "util.h"
#include "server-config.h"

#define ERRNO_BUF_SIZE 128 


static pthread_mutex_t logging_lock = PTHREAD_MUTEX_INITIALIZER;

char errno_buf[ERRNO_BUF_SIZE];

void log_message(FILE *const log_file, const char *msg)
{
    char time[TIME_BUFFER_SIZE];
    getTime(time, TIME_BUFFER_SIZE);

    pthread_mutex_lock(&logging_lock);
    fprintf(log_file, "%s %s\n", time, msg);
    pthread_mutex_unlock(&logging_lock);
    fflush(log_file);
}

void log_net_data(FILE *const log_file, const char *buf, size_t buf_size)
{
    char time[TIME_BUFFER_SIZE];
    getTime(time, TIME_BUFFER_SIZE);

    pthread_mutex_lock(&logging_lock);

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

    pthread_mutex_unlock(&logging_lock);
    fflush(log_file);
}


void log_error(FILE *const log_file, const char *msg, int errno_code)
{
    char time[TIME_BUFFER_SIZE];
    getTime(time, TIME_BUFFER_SIZE);

    pthread_mutex_lock(&logging_lock);

    if(errno_code == 0)
        fprintf(log_file, "%s %s\n", time, msg);
    else
        fprintf(log_file, "%s %s: %s\n", time, msg, strerror_r(errno_code, errno_buf, ERRNO_BUF_SIZE));

    pthread_mutex_unlock(&logging_lock);
    fflush(log_file);
}

void log_error_fd(FILE *const log_file, const char *err_msg, int conn_fd, int errno_code)
{
    char time[TIME_BUFFER_SIZE]; 
    getTime(time, TIME_BUFFER_SIZE);

    pthread_mutex_lock(&logging_lock);

    if(errno_code == 0)
        fprintf(log_file, "%s %s fd (%d)\n", time, err_msg, conn_fd);
    else
        fprintf(log_file, "%s %s fd (%d): %s\n", time, err_msg, conn_fd, strerror_r(errno_code, errno_buf, ERRNO_BUF_SIZE));

    pthread_mutex_unlock(&logging_lock);
    fflush(log_file);
}