#ifndef _PORT_MANAGER_H
#define _PORT_MANAGER_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "ds/ds_queue.h"
#include "ds/hashmap.h"

#define QUEUE_CAPACITY 128
#define HT_CAPACITY 128

struct PortManager
{
    Queue *port_queue; 

    // Reaper Thread Fields 
    pthread_t reaper_thread;
    pthread_mutex_t ports_lock;         // mutex used when interfering with queue with ports
    _Atomic bool reaper_thread_active;  // Indicate if reaper thread active
    _Atomic bool reaper_thread_stop;    // Flag set by parent to request child thread exit 
    _Atomic uint8_t reaper_exit_status; // Holds reaper thread exit status

    // Hashmap (pid_t -> port) <Later will be moved to a separate matchmaker module> 
    HashTable *pid_to_port_table; 
    pthread_mutex_t ht_lock;
};

struct PortManager *initPortManager(FILE *const log_file);

void destroyPortManager(struct PortManager *pm, FILE *const log_file);

bool pm_ready(struct PortManager *pm, FILE *const log_file);

uint16_t getPort(struct PortManager *pm, FILE *const log_file);

#endif 