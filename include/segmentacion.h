#ifndef SEGMENTACION_H
#define SEGMENTACION_H

#include "simulator.h"

typedef struct {
    uint64_t base;
    uint64_t limit;
} segment_entry_t;

typedef struct {
    segment_entry_t *segments;
    int              num_segments;
} segment_table_t;

void seg_init(segment_table_t *st, int thread_id);
void seg_destroy(segment_table_t *st);
int  seg_translate(segment_table_t *st, int seg_id, uint64_t offset,
                   uint64_t *pa_out, metrics_t *m);
void seg_run_thread(int thread_id, metrics_t *m);

#endif
