#ifndef TLB_H
#define TLB_H

#include "simulator.h"

typedef struct {
    uint64_t vpn;
    uint64_t frame_number;
    int      valid;
} tlb_entry_t;

typedef struct {
    tlb_entry_t *entries;
    int          size;
    int          next_index;
    int          count;
} tlb_t;

void    tlb_init(tlb_t *tlb, int size);
void    tlb_destroy(tlb_t *tlb);
int     tlb_lookup(tlb_t *tlb, uint64_t vpn, uint64_t *frame_out);
void    tlb_insert(tlb_t *tlb, uint64_t vpn, uint64_t frame_number);
void    tlb_invalidate(tlb_t *tlb, uint64_t vpn);

#endif
