#include "orchestrator/core/port-manager/port_manager.h"
#include "log_system.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>

#define REAPER_FAILURE 1
#define REAPER_NORMAL 0

#define INVALID_PORT 0

// ===================================== Internal HASHTABLE HELPERS ==================================================== 

static unsigned int pid_hash(const void *key, unsigned int table_size)
{
    pid_t pid = *(const pid_t *)key;

    uint32_t x = (uint32_t)pid;

    /* bit mixing */
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;

    return x % table_size;
}

static uint16_t ht_retrieve_port(struct PortManager *pm, pid_t process_id, 
                                 FILE *const log_file)
{
    uint16_t *port_ptr = NULL;
    uint16_t port; 
    int rs = 0; 

    pthread_mutex_lock(&pm->ht_lock);

    port_ptr = ht_get(pm->pid_to_port_table, process_id, sizeof(pid_t));
    if(!port_ptr)
    {
        pthread_mutex_unlock(&pm->ht_lock);
        log_message(log_file, "[ht_retrieve_port] ht_get fail.");
        return INVALID_PORT;
    }

    port = *port_ptr;

    rs = ht_remove(pm->pid_to_port_table, process_id);
    pthread_mutex_unlock(&pm->ht_lock);
    
    if(rs < 0)
    {
        log_message(log_file, "[ht_retrieve_port] ht_remove.");
        return INVALID_PORT;
    }

    return port;
}

static int ht_insert_port(struct PortManager *pm, pid_t process_id, 
                          uint16_t port, FILE *const log_file)
{
    int rc;

    pthread_mutex_lock(&pm->ht_lock);
    rc = ht_insert(pm->pid_to_port_table, process_id, port);
    pthread_mutex_unlock(&pm->ht_lock);

    if(rc < 0)
    {
        log_message(log_file, "[ht_insert_port] ht_insert failed.");
        return -1;
    }

    return 0;
}

// ================== Internal Wrappers QUEUE =================
static size_t get_queue_size(struct PortManager *pm)
{
    size_t cur_size; 
    pthread_mutex_lock(&pm->ports_lock);
    cur_size = q_size(pm->port_queue);
    pthread_mutex_unlock(&pm->ports_lock);
    return cur_size;
}

static uint16_t pop_queue(struct PortManager *pm, FILE *const log_file)
{
    uint16_t port = 0; 
    pthread_mutex_lock(&pm->ports_lock);
    int rc = q_dequeue(pm->port_queue, &port);
    pthread_mutex_unlock(&pm->ports_lock);

    if(rc != 0)
    {
        log_message(log_file, "[pop_queue] q_dequeue failed.");
        return INVALID_PORT;
    }

    return port; 
}

static int push_queue(struct PortManager *pm, uint16_t port, 
                      FILE *const log_file)
{
    pthread_mutex_lock(&pm->ports_lock);
    int rc = q_enqueue(pm->port_queue, uint16_t, port);
    pthread_mutex_unlock(&pm->ports_lock);

    if(rc != 0)
    {
        log_message(log_file, "[push_queue] q_enqueue failed.");
        return -1;
    }
    
    return 0;
}

struct ReaperArgs
{
    struct PortManager *pm;
    FILE *log_file;
};

static void *reaper_thread(void *args)
{
    struct ReaperArgs *r_args = (struct ReaperArgs *)args;

    sigset_t set; 
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);

    struct timespec ts; 
    ts.tv_sec = 1; 
    ts.tv_nsec = 0;

    while(!r_args->pm->reaper_thread_stop)
    {
        int sig = sigtimedwait(&set, NULL, &ts);    
        if(sig == SIGCHLD)
        {
            for(;;)
            {
                pid_t pid = waitpid(-1, NULL, WNOHANG);
                if(pid == 0) break;

                if(pid < 0)
                {
                    if(errno ==  ECHILD) break;

                    log_message(r_args->log_file, "[reaper_thread] waitpid failed.");
                    r_args->pm->reaper_thread_active = false;
                    r_args->pm->reaper_exit_status = EXIT_FAILURE;
                    free(args);
                    return NULL;                   
                } 
                
                uint16_t port = ht_retrieve_port(r_args->pm, pid, r_args->log_file);
                if(port == INVALID_PORT)
                {
                    log_message(r_args->log_file, "[reaper_thread] ht_get_port failed.");
                    r_args->pm->reaper_thread_active = false; 
                    r_args->pm->reaper_exit_status = EXIT_FAILURE;
                    free(args);
                    return NULL;
                }

                if(push_queue(r_args->pm, port, r_args->log_file) < 0)
                {
                    log_message(r_args->log_file, "[reaper_thread] push_queue failed.");
                    r_args->pm->reaper_thread_active = false; 
                    r_args->pm->reaper_exit_status = EXIT_FAILURE;
                    free(args);
                    return NULL;
                }               
            }
        }
        else if(sig == -1 && (errno == EINTR || errno == EAGAIN)) continue; 
        else
        {
            log_message(r_args->log_file, "[reaper_thread] sigtimedwait critical failure.");
            r_args->pm->reaper_thread_active = false; 
            r_args->pm->reaper_exit_status = EXIT_FAILURE;
            free(args);
            return NULL;
        }
    }

    r_args->pm->reaper_thread_active = false; 
    r_args->pm->reaper_exit_status = EXIT_SUCCESS;
    free(args);
    return NULL;
}

struct PortManager *initPortManager(FILE *const log_file)
{
    struct PortManager *pm = calloc(1, sizeof(*pm));
    if(!pm)
    {
        log_message(log_file, "[initPortManager] calloc failed.");
        return NULL;
    }

    pm->port_queue = q_init(QUEUE_CAPACITY, sizeof(uint16_t));
    if(!pm->port_queue)
    {
        log_message(log_file, "[initPortManager] q_init failed.");
        free(pm);
        return NULL;
    }

    pm->pid_to_port_table = ht_create(sizeof(pid_t), 1, sizeof(uint16_t), 1, 
                                      pid_hash, HT_CAPACITY);

    if(!pm->pid_to_port_table)
    {
        log_message(log_file, "[initPortManager] ht_create failed.");
        q_destroy(pm->port_queue);
        free(pm);
        return NULL;
    }

    if(pthread_mutex_init(&pm->ht_lock, NULL) != 0)
    {
        log_message(log_file, "[initPortManager] pthread_mutex_init failed.");
        q_destroy(pm->port_queue);
        ht_destroy(pm->pid_to_port_table);
        free(pm);
        return NULL;
    }

    if(pthread_mutex_init(&pm->ports_lock, NULL) != 0)
    {
        log_message(log_file, "[initPortManager] pthread_mutex_init failed.");
        q_destroy(pm->port_queue);
        ht_destroy(pm->pid_to_port_table);
        pthread_mutex_destroy(&pm->ht_lock);
        free(pm);
        return NULL;
    }

    struct ReaperArgs *args = malloc(sizeof(*args));
    if(!args)
    {
        log_message(log_file, "[initPortManager] ReaperArgs malloc failed.");
        q_destroy(pm->port_queue);
        pthread_mutex_destroy(&pm->ports_lock);
        pthread_mutex_destroy(&pm->ht_lock);
        ht_destroy(pm->pid_to_port_table);
        free(pm);
        return NULL;
    }

    args->pm = pm; 
    args->log_file = log_file;

    pm->reaper_thread_active = true;
    pm->reaper_thread_stop = false; 
    pm->reaper_exit_status = 0;

    if(pthread_create(&pm->reaper_thread, NULL, reaper_thread, args) != 0)
    {
        log_message(log_file, "[initPortManager] pthread_create failed.");
        free(args);
        q_destroy(pm->port_queue);
        ht_destroy(pm->pid_to_port_table);
        pthread_mutex_destroy(&pm->ports_lock);
        free(pm);
        return NULL;
    }

    return pm;
}

void destroyPortManager(struct PortManager *pm, FILE *const log_file)
{
    // Stop reaper thread
    pm->reaper_thread_stop = true;
    pthread_join(pm->reaper_thread, NULL);

    if(pm->reaper_exit_status == REAPER_FAILURE)
    {
        char err[128];
        snprintf(err, 128, "[destroyPortManager] reaper thread exited \
                            with failure code: %hhu", pm->reaper_exit_status);
        log_message(log_file, err);
    }

    pthread_mutex_destroy(&pm->ports_lock);
    q_destroy(pm->port_queue);
    free(pm);
}   

bool pm_ready(struct PortManager *pm, FILE *const log_file)
{
    (void)log_file; // silence compiler warning 
    return get_queue_size(pm) > 0;
}

uint16_t getPort(struct PortManager *pm, FILE *const log_file)
{
    return pop_queue(pm, log_file);
}