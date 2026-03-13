#ifndef WORKLOADS_H
#define WORKLOADS_H

#include "simulator.h"

void     wl_gen_seg(unsigned *rng, int *seg_id, uint64_t *offset);
uint64_t wl_gen_page(unsigned *rng);

#endif
