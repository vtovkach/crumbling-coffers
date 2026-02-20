#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stddef.h>
#include <time.h>

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