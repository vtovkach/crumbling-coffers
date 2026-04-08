#include "matchmaker/port_manager.h"
#include "ds/hashmap.h"
#include "ds/spsc_queue.h"
#include "log_system.h"

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define PM_HT_INITIAL_SIZE 100

struct PortManager
{
    FILE *log_file;

    SPSCQueue       *ports;
    pthread_mutex_t  ports_lock;

    pthread_t    reaper_thread;
    atomic_bool  reaper_active;
    atomic_bool  reaper_stop;

    HashTable       *pid_port_mapping;
    pthread_mutex_t  ht_lock;
};

static unsigned int hash_pid(const void *key, unsigned int table_size)
{
    uint32_t x;
    memcpy(&x, key, sizeof(uint32_t));

    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;

    return x % table_size;
}

static uint16_t pm_retrieve_port(struct PortManager *pm, pid_t pid)
{
    pthread_mutex_lock(&pm->ht_lock);

    uint16_t *port_ptr = ht__get_internal(pm->pid_port_mapping, &pid, sizeof(pid_t));
    if (!port_ptr)
    {
        pthread_mutex_unlock(&pm->ht_lock);
        return 0;
    }

    uint16_t port = *port_ptr;
    ht__remove_internal(pm->pid_port_mapping, &pid);
    pthread_mutex_unlock(&pm->ht_lock);

    return port;
}

static void *reaper_thread(void *args)
{
    struct PortManager *pm = (struct PortManager *) args;
    atomic_store(&pm->reaper_active, true);

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);

    struct timespec ts;
    ts.tv_sec  = 1;
    ts.tv_nsec = 0;

    while (!atomic_load(&pm->reaper_stop))
    {
        int sig = sigtimedwait(&set, NULL, &ts);
        if (sig == SIGCHLD)
        {
            for (;;)
            {
                pid_t pid = waitpid(-1, NULL, WNOHANG);
                if (pid == 0) break;

                if (pid < 0)
                {
                    if (errno == ECHILD) break;

                    log_error(pm->log_file, "[reaper_thread] waitpid failed", errno);
                    atomic_store(&pm->reaper_active, false);
                    return NULL;
                }

                uint16_t port = pm_retrieve_port(pm, pid);
                if (port == 0)
                {
                    log_error(pm->log_file, "[reaper_thread] pm_retrieve_port: unknown pid, skipping", 0);
                    continue;
                }

                pthread_mutex_lock(&pm->ports_lock);
                int rc = spscq_push(pm->ports, &port);
                pthread_mutex_unlock(&pm->ports_lock);

                if (rc != 0)
                {
                    log_error(pm->log_file, "[reaper_thread] failed to return port to queue", 0);
                    atomic_store(&pm->reaper_active, false);
                    return NULL;
                }

                char msg[64]; 
                snprintf(msg, 64, "[reaper_thread] port %u is retrieved back", port);
                log_message(pm->log_file, msg);
            }
        }
        else if (sig == -1 && (errno == EINTR || errno == EAGAIN)) continue;
        else
        {
            log_error(pm->log_file, "[reaper_thread] sigtimedwait critical failure", errno);
            atomic_store(&pm->reaper_active, false);
            return NULL;
        }
    }

    atomic_store(&pm->reaper_active, false);
    return NULL;
}

struct PortManager *pm_create(uint16_t *initial_ports,
                              size_t init_ports_size,
                              FILE *log_file)
{
    sigset_t chld_set;
    sigemptyset(&chld_set);
    sigaddset(&chld_set, SIGCHLD);
    if (pthread_sigmask(SIG_BLOCK, &chld_set, NULL) != 0)
    {
        log_error(log_file, "pm_create: failed to block SIGCHLD", errno);
        return NULL;
    }

    struct PortManager *pm = calloc(1, sizeof(*pm));
    if (!pm)
    {
        log_error(log_file, "pm_create: failed to allocate PortManager", errno);
        return NULL;
    }

    pm->log_file = log_file;

    pm->ports = spscq_create(init_ports_size + 1, sizeof(uint16_t));
    if (!pm->ports)
    {
        log_error(log_file, "pm_create: failed to create ports queue", errno);
        free(pm);
        return NULL;
    }

    for (size_t i = 0; i < init_ports_size; i++)
    {
        if (spscq_push(pm->ports, &initial_ports[i]) != 0)
        {
            log_error(log_file, "pm_create: failed to push initial port into queue", 0);
            spscq_destroy(pm->ports);
            free(pm);
            return NULL;
        }
    }

    if (pthread_mutex_init(&pm->ports_lock, NULL) != 0)
    {
        log_error(log_file, "pm_create: failed to init ports_lock", errno);
        spscq_destroy(pm->ports);
        free(pm);
        return NULL;
    }

    pm->pid_port_mapping = ht_create(sizeof(pid_t), 1, sizeof(uint16_t), 1,
                                     hash_pid, PM_HT_INITIAL_SIZE);
    if (!pm->pid_port_mapping)
    {
        log_error(log_file, "pm_create: failed to create pid_port_mapping", 0);
        pthread_mutex_destroy(&pm->ports_lock);
        spscq_destroy(pm->ports);
        free(pm);
        return NULL;
    }

    if (pthread_mutex_init(&pm->ht_lock, NULL) != 0)
    {
        log_error(log_file, "pm_create: failed to init ht_lock", errno);
        ht_destroy(pm->pid_port_mapping);
        pthread_mutex_destroy(&pm->ports_lock);
        spscq_destroy(pm->ports);
        free(pm);
        return NULL;
    }

    atomic_init(&pm->reaper_active, false);
    atomic_init(&pm->reaper_stop, false);

    if (pthread_create(&pm->reaper_thread, NULL, reaper_thread, pm) != 0)
    {
        log_error(log_file, "pm_create: failed to start reaper thread", errno);
        pthread_mutex_destroy(&pm->ht_lock);
        ht_destroy(pm->pid_port_mapping);
        pthread_mutex_destroy(&pm->ports_lock);
        spscq_destroy(pm->ports);
        free(pm);
        return NULL;
    }

    return pm;
}

void pm_destroy(struct PortManager *pm)
{
    if (!pm) return;

    atomic_store(&pm->reaper_stop, true);
    pthread_join(pm->reaper_thread, NULL);

    ht_destroy(pm->pid_port_mapping);
    pthread_mutex_destroy(&pm->ht_lock);
    spscq_destroy(pm->ports);
    pthread_mutex_destroy(&pm->ports_lock);
    free(pm);
}

uint16_t pm_borrow_port(struct PortManager *pm)
{
    if (!pm) return 0;

    uint16_t port = 0;
    pthread_mutex_lock(&pm->ports_lock);
    spscq_pop(pm->ports, &port);
    pthread_mutex_unlock(&pm->ports_lock);

    return port;
}

void pm_return_port(struct PortManager *pm, uint16_t port)
{
    if (!pm) return;

    pthread_mutex_lock(&pm->ports_lock);
    if (spscq_push(pm->ports, &port) != 0)
        log_error(pm->log_file, "pm_return_port: failed to return port to queue", 0);
    pthread_mutex_unlock(&pm->ports_lock);
}

int pm_register_port(struct PortManager *pm, pid_t pid, uint16_t port)
{
    if (!pm) return -1;

    pthread_mutex_lock(&pm->ht_lock);
    int ret = ht__insert_internal(pm->pid_port_mapping, &pid, &port);
    pthread_mutex_unlock(&pm->ht_lock);

    if (ret != 0)
    {
        log_error(pm->log_file, "pm_register_port: failed to insert pid-port mapping", 0);
        return -1;
    }

    return 0;
}

bool pm_is_port(struct PortManager *pm)
{
    if (!pm) return false;

    pthread_mutex_lock(&pm->ports_lock);
    bool available = !spscq_is_empty(pm->ports);
    pthread_mutex_unlock(&pm->ports_lock);

    return available;
}

bool pm_status(struct PortManager *pm)
{
    if (!pm) return false;
    return atomic_load(&pm->reaper_active);
}
