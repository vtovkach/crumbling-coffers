#ifndef _PORT_MANAGER
#define _PORT_MANAGER

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>

struct PortManager; 

/**
 * Creates and initializes a PortManager.
 *
 * @param initial_ports     Array of available ports.
 * @param init_ports_size   Number of ports in the array.
 * @return                  Pointer to PortManager on success, NULL on
 *                          failure.
 */
struct PortManager *pm_create(uint16_t *initial_ports,
                              size_t init_ports_size,
                              FILE * log_file);

/**
 * Destroys a PortManager and releases all associated resources.
 *
 * @param pm  Pointer to the PortManager to destroy.
 */
void pm_destroy(struct PortManager *pm);

/**
 * Borrows an available port from the manager.
 *
 * @param pm  Pointer to the PortManager.
 * @return    Port number on success, 0 if no ports are available.
 */
uint16_t pm_borrow_port(struct PortManager *pm);

/**
 * Returns a previously borrowed port back to the manager.
 *
 * Intended for cases where a port was borrowed but not used
 * (e.g., game process creation failed).
 *
 * @param pm    Pointer to the PortManager.
 * @param port  Port to return.
 */
void pm_return_port(struct PortManager *pm, uint16_t port);

/**
 * Registers a port with a process after successful usage.
 *
 * Associates the given PID with the port, indicating the port
 * is actively in use.
 *
 * @param pm    Pointer to the PortManager.
 * @param pid   Process ID using the port.
 * @param port  Port to register.
 * @return      0 on success, -1 on failure.
 */
int pm_register_port(struct PortManager *pm,
                     pid_t pid,
                     uint16_t port);

/**
 * Checks whether at least one port is available.
 *
 * @param pm  Pointer to the PortManager.
 * @return    true if a port is available, false otherwise.
 */
bool pm_is_port(struct PortManager *pm);

/**
 * Checks whether the PortManager is operating normally.
 *
 * @param pm  Pointer to the PortManager.
 * @return    true if the manager is healthy, false if a critical
 *            error has occurred.
 */
bool pm_status(struct PortManager *pm);

#endif