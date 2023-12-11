/**
 * Tony Givargis
 * Copyright (C), 2023
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * main.c
 */

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "system.h"

/**
 * Needs:
 *   signal()
 */

static volatile int done;

static void
_signal_(int signum)
{
    assert(SIGINT == signum);

    done = 1;
}

double
cpu_util(const char *s)
{
    static unsigned sum_, vector_[7];
    unsigned sum, vector[7];
    const char *p;
    double util;
    uint64_t i;

    /*
        user
        nice
        system
        idle
        iowait
        irq
        softirq
    */

    if (!(p = strstr(s, " ")) ||
        (7 != sscanf(p,
                    "%u %u %u %u %u %u %u",
                    &vector[0],
                    &vector[1],
                    &vector[2],
                    &vector[3],
                    &vector[4],
                    &vector[5],
                    &vector[6]))) {
        return 0;
    }
    sum = 0.0;
    for (i = 0; i < ARRAY_SIZE(vector); ++i)
    {
        sum += vector[i];
    }
    util = (1.0 - (vector[3] - vector_[3]) / (double)(sum - sum_)) * 100.0;
    sum_ = sum;
    for (i = 0; i < ARRAY_SIZE(vector); ++i)
    {
        vector_[i] = vector[i];
    }
    return util;
}

double calculate_memory_utilization() {
    const char * const PROC_MEMINFO = "/proc/meminfo";
    char line[1024];
    FILE *file;
    unsigned long total_memory = 0, free_memory = 0, active_memory = 0;

    if (!(file = fopen(PROC_MEMINFO, "r"))) {
        TRACE("fopen()");
        return -1.0;
    }

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "MemTotal")) {
            sscanf(line, "%*s %lu", &total_memory);
        } else if (strstr(line, "MemFree")) {
            sscanf(line, "%*s %lu", &free_memory);
        } else if (strstr(line, "Active")) {
            sscanf(line, "%*s %lu", &active_memory);
        }
    }

    fclose(file);

    if (total_memory == 0) {
        TRACE("Failed to read total memory from /proc/meminfo");
        return -1.0;
    }

    return ((double)(total_memory - free_memory) / total_memory) * 100.0;
}

double calculate_network_utilization() {
    const char * const PROC_NET_DEV = "/proc/net/dev";
    char line[1024];
    FILE *file;
    unsigned long rx_bytes = 0, tx_bytes = 0;
    static unsigned tx_bytes_prev = 0, rx_bytes_prev = 0;
    double result = 0.0;

    if (!(file = fopen(PROC_NET_DEV, "r"))) {
        TRACE("fopen()");
        return -1.0;
    }

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "enp0s1")) {
            sscanf(line, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %lu %*s %*s %*s %*s %*s %*s %*s %lu", &rx_bytes, &tx_bytes);
        }
    }

    result = ((double)((rx_bytes - rx_bytes_prev) + (tx_bytes - tx_bytes_prev))) / 1024.0;
    rx_bytes_prev = rx_bytes;
    tx_bytes_prev = tx_bytes;

    fclose(file);

    return result;
}

double calculate_io_utilization() {
    const char * const PROC_DISKSTATS = "/proc/diskstats";
    char line[1024];
    FILE *file;
    unsigned long read_sectors = 0, write_sectors = 0;

    if (!(file = fopen(PROC_DISKSTATS, "r"))) {
        TRACE("fopen()");
        return -1.0;
    }

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "sda")) {
            sscanf(line, "%*u %*u sda %*u %*u %lu %*u %*u %*u %lu", &read_sectors, &write_sectors);
        }
    }

    fclose(file);

    return ((double)(read_sectors + write_sectors) / 2);
}

int main(int argc, char *argv[]) {
    const char * const PROC_STAT = "/proc/stat";
    char line[1024];
    FILE *file;

    UNUSED(argc);
    UNUSED(argv);

    if (SIG_ERR == signal(SIGINT, _signal_)) {
        TRACE("signal()");
        return -1;
    }

    while (!done) {
        if (!(file = fopen(PROC_STAT, "r"))) {
            TRACE("fopen()");
            return -1;
        }

        if (fgets(line, sizeof(line), file)) {
            printf("\rCPU Utilization: %5.1f%%, Memory Utilization: %5.1f%%, Network Utilization: %5.1f KB, I/O Utilization: %7.1f      ",
                   cpu_util(line), calculate_memory_utilization(), calculate_network_utilization(), calculate_io_utilization());
            fflush(stdout);
        }

        us_sleep(500000);
        fclose(file);
    }

    printf("\nDone!   \n");
    return 0;
}
