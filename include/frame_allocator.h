#ifndef FRAME_ALLOCATOR_H
#define FRAME_ALLOCATOR_H

#include "simulator.h"

/* Nodo de la cola FIFO de páginas en memoria */
typedef struct fifo_node {
    uint64_t         frame;
    uint64_t         vpn;
    int              thread_id;
    struct fifo_node *next;
} fifo_node_t;

typedef struct {
    int              num_frames;
    int             *free_list;
    fifo_node_t     *head;
    fifo_node_t     *tail;
    pthread_mutex_t  lock;
} frame_allocator_t;

void     fa_init(frame_allocator_t *fa, int num_frames);
void     fa_destroy(frame_allocator_t *fa);
uint64_t fa_alloc(frame_allocator_t *fa, uint64_t vpn, int tid,
                  uint64_t *evicted_vpn, int *evicted_tid, int *did_evict);

#endif
