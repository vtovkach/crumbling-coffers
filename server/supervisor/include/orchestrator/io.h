#ifndef _IO_H
#define _IO_H

#include <stdio.h>

/**
 * tcp_read - Reads data from a TCP socket into the provided buffer.
 *
 * @log_file: File handle used for logging errors 
 * @fd: File descriptor of the connected TCP socket
 * @dest: Destination buffer to store received data
 * @dest_size: Size of the destination buffer in bytes
 *
 * Behavior:
 *   - Attempts to read up to dest_size bytes from the socket.
 *   - Due to non-blocking architecture, returns immediately if no data is available.
 *   - Caller is responsible for retrying on next socket readiness event.
 *
 * Return:
 *   > 0  : Number of bytes successfully read into the buffer
 *   0    : No data available (EAGAIN / EWOULDBLOCK / EINTR on non-blocking socket)
 *  -1    : Error occurred (details logged via log_file if provided)
 *  -2    : Peer has closed the connection (graceful disconnect)
 *
 * Notes:
 *   - This function does not guarantee that the buffer is fully filled.
 *   - Designed for use with event-driven I/O (epoll, kqueue, select).
 */
int tcp_read(FILE *log_file, int fd, void *dest, size_t dest_size);

/**
 * tcp_send - Send data over a TCP socket
 *
 * Sends buf_size bytes from buf to the specified socket descriptor.
 * Handles non-blocking socket behavior and transient errors gracefully.
 *
 * @log_file: Log file handle for error reporting
 * @fd:       Socket file descriptor
 * @buf:      Data buffer to send (must not be NULL)
 * @buf_size: Number of bytes to send (must be > 0)
 *
 * @return:   On success, returns the number of bytes sent (>= 0).
 *            On transient error (EINTR, EAGAIN, EWOULDBLOCK), returns 0
 *            and caller should retry on next socket readiness event.
 *            On fatal error, returns -1 and logs error details.
 *
 * Note: This function is designed for use with non-blocking sockets and
 * event-driven I/O (epoll, kqueue, select). Transient errors are not
 * retried internally; the caller's event loop will handle re-invocation
 * when the socket is ready.
 */
int tcp_send(FILE *log_file, int fd, void *buf, size_t buf_size);

#endif