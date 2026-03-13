#ifndef PAGINACION_H
#define PAGINACION_H

#include "simulator.h"
#include "tlb.h"
#include "frame_allocator.h"

typedef struct {
    uint64_t frame_number;
    int      valid;
} page_table_entry_t;

typedef struct {
    page_table_entry_t *entries;
    int                 num_pages;
} page_table_t;

typedef struct {
    int          thread_id;
    page_table_t pt;
    tlb_t        tlb;
} page_state_t;

extern frame_allocator_t  g_fa;
extern page_state_t      *g_page_states;

void page_system_init(int num_threads);
void page_system_destroy(int num_threads);
void page_run_thread(int thread_id, metrics_t *m);

#endif
