/*
 * test_port_reaper.c
 *
 * Local unit test for the PortManager reaper thread.
 * Does NOT require a running server — calls the pm_* API directly.
 *
 * Tests:
 *   1. Port is reclaimed after a registered child exits (N cycles to
 *      confirm reuse works across multiple matches).
 *   2. Reaper thread survives an unregistered pid (does not die on
 *      unknown children).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#include "matchmaker/port_manager.h"

#define TEST_TIMEOUT_S  60
#define REAP_WAIT_US    1500000   /* 1.5s — reaper polls on a 1s timeout */
#define N_CYCLES        20
#define TEST_PORT       20000

static void on_timeout(int sig)
{
    (void)sig;
    fprintf(stderr, "FAIL: test timed out after %d seconds\n", TEST_TIMEOUT_S);
    exit(EXIT_FAILURE);
}

/* Forks a child that exits immediately and returns its pid. */
static pid_t spawn_dummy(void)
{
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }
    if (pid == 0) _exit(0);
    return pid;
}

int main(void)
{
    signal(SIGALRM, on_timeout);
    alarm(TEST_TIMEOUT_S);

    printf("=== test_port_reaper ===\n\n");

    /* Single-port pool so exhaustion and reclaim are easy to observe. */
    uint16_t initial_ports[] = { TEST_PORT };
    struct PortManager *pm = pm_create(initial_ports, 1, stderr);
    if (!pm)
    {
        fprintf(stderr, "FAIL: pm_create returned NULL\n");
        return 1;
    }

    /* Give the reaper thread a moment to start and set reaper_active. */
    usleep(100000);

    if (!pm_status(pm))
    {
        fprintf(stderr, "FAIL: reaper thread did not become active\n");
        pm_destroy(pm);
        return 1;
    }

    printf("Reaper thread is active.\n");

    /* ---------------------------------------------------------------
     * Test 1: port is reclaimed and reusable across N match cycles.
     * --------------------------------------------------------------- */
    printf("\nTest 1: port reclaim and reuse (%d cycles)\n", N_CYCLES);

    for (int cycle = 0; cycle < N_CYCLES; cycle++)
    {
        if (!pm_is_port(pm))
        {
            fprintf(stderr, "FAIL: cycle %d: no port available at cycle start\n", cycle);
            pm_destroy(pm);
            return 1;
        }

        uint16_t port = pm_borrow_port(pm);
        if (port == 0)
        {
            fprintf(stderr, "FAIL: cycle %d: pm_borrow_port returned 0\n", cycle);
            pm_destroy(pm);
            return 1;
        }

        if (pm_is_port(pm))
        {
            fprintf(stderr, "FAIL: cycle %d: pool not empty after borrowing sole port\n", cycle);
            pm_destroy(pm);
            return 1;
        }

        pid_t pid = spawn_dummy();

        if (pm_register_port(pm, pid, port) < 0)
        {
            fprintf(stderr, "FAIL: cycle %d: pm_register_port failed\n", cycle);
            pm_destroy(pm);
            return 1;
        }

        printf("  cycle %d: port=%u pid=%d — waiting for reap...\n",
               cycle, (unsigned)port, (int)pid);

        usleep(REAP_WAIT_US);

        if (!pm_status(pm))
        {
            fprintf(stderr, "FAIL: cycle %d: reaper thread died after reaping\n", cycle);
            pm_destroy(pm);
            return 1;
        }

        if (!pm_is_port(pm))
        {
            fprintf(stderr,
                    "FAIL: cycle %d: port=%u was not returned to pool after child exited\n",
                    cycle, (unsigned)port);
            pm_destroy(pm);
            return 1;
        }

        printf("  cycle %d: port=%u reclaimed successfully. PASS\n", cycle, (unsigned)port);
    }

    /* ---------------------------------------------------------------
     * Test 2: reaper thread survives a child it never registered.
     * --------------------------------------------------------------- */
    printf("\nTest 2: reaper survives unregistered pid\n");

    pid_t unknown = spawn_dummy();
    /* Intentionally do NOT call pm_register_port for this pid. */

    printf("  spawned unregistered pid=%d — waiting...\n", (int)unknown);
    usleep(REAP_WAIT_US);

    if (!pm_status(pm))
    {
        fprintf(stderr, "FAIL: reaper thread died on unregistered pid\n");
        pm_destroy(pm);
        return 1;
    }

    /* Pool should still have the port from the last cycle. */
    if (!pm_is_port(pm))
    {
        fprintf(stderr, "FAIL: port disappeared after unknown-pid reap\n");
        pm_destroy(pm);
        return 1;
    }

    printf("  reaper survived unregistered pid, pool intact. PASS\n");

    pm_destroy(pm);
    printf("\nPASS: all tests passed\n");
    return 0;
}
