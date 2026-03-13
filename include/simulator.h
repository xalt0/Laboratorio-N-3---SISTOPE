#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── Modos ─────────────────────────────────────────────── */
#define MODE_SEG  0
#define MODE_PAGE 1

#define WL_UNIFORM 0
#define WL_8020    1

#define INVALID_FRAME UINT64_MAX

/* ── Configuración global ──────────────────────────────── */
typedef struct {
    int      mode;
    int      num_threads;
    int      ops_per_thread;
    int      workload;
    unsigned seed;
    int      unsafe;
    int      show_stats;

    /* Segmentación */
    int      num_segments;
    uint64_t seg_limits[64];

    /* Paginación */
    int      num_pages;
    int      num_frames;
    int      page_size;
    int      tlb_size;
} config_t;

/* ── Métricas por thread ───────────────────────────────── */
typedef struct {
    long   translations_ok;
    long   segfaults;
    long   tlb_hits;
    long   tlb_misses;
    long   page_faults;
    long   evictions;
    long   total_ops;
    double total_time_ns;
} metrics_t;

/* ── Métricas globales ─────────────────────────────────── */
typedef struct {
    long            translations_ok;
    long            segfaults;
    long            tlb_hits;
    long            tlb_misses;
    long            page_faults;
    long            evictions;
    long            total_ops;
    pthread_mutex_t lock;
} global_metrics_t;

extern config_t        g_cfg;
extern global_metrics_t g_met;

/* ── Tiempo ────────────────────────────────────────────── */
static inline double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + ts.tv_nsec;
}

#endif
