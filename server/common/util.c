#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stddef.h>
#include <time.h>
#include <string.h>

#include "../include/common/util.h"

int redirect_stderr(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(fd == -1)
        return -1;
        
    // Redirect STDERR (fd 2) to custom log file
    if(dup2(fd, STDERR_FILENO) == -1)
        return -1;
    
    close(fd);

    return 0;
}   

void getTime(char *dest, size_t buf_size)
{
    time_t now = time(NULL);

    struct tm tm_info;
    localtime_r(&now, &tm_info);

    // Parse time into a string
    strftime(dest, buf_size, "%Y-%m-%d %H:%M:%S", &tm_info);
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

void log_error(FILE *const log_file, const char *err_msg, int errno_code)
{
    char time[TIME_BUFFER_SIZE]; 
    getTime(time, TIME_BUFFER_SIZE);

    if(errno_code == 0)
        fprintf(log_file, "%s %s\n", time, err_msg);
    else
        fprintf(log_file, "%s %s: %s\n", time, err_msg, strerror(errno_code));
}