#include "workloads.h"

/* Genera (seg_id, offset) para modo segmentación.
 * Uniform: todo aleatorio.
 * 80-20: 80% de accesos al 20% inferior de segmentos. */
void wl_gen_seg(unsigned *rng, int *seg_id, uint64_t *offset) {
    int n = g_cfg.num_segments;
    int seg;

    if (g_cfg.workload == WL_8020) {
        int hot = (n * 20 + 99) / 100;
        if (hot < 1) hot = 1;
        double r = (rand_r(rng) % 100) / 100.0;
        seg = (r < 0.80) ? rand_r(rng) % hot
                         : hot + rand_r(rng) % (n - hot);
    } else {
        seg = rand_r(rng) % n;
    }

    /* Generamos offsets que a veces superan el límite para producir segfaults */
    uint64_t lim = g_cfg.seg_limits[seg];
    *seg_id  = seg;
    *offset  = (uint64_t)rand_r(rng) % (lim + lim / 2 + 1);
}

/* Genera una VA para modo paginación.
 * Uniform: VPN aleatorio.
 * 80-20: 80% de accesos al 20% inferior de VPNs. */
uint64_t wl_gen_page(unsigned *rng) {
    int n   = g_cfg.num_pages;
    int psz = g_cfg.page_size;
    int vpn;

    if (g_cfg.workload == WL_8020) {
        int hot = (n * 20 + 99) / 100;
        if (hot < 1) hot = 1;
        double r = (rand_r(rng) % 100) / 100.0;
        vpn = (r < 0.80) ? rand_r(rng) % hot
                         : hot + rand_r(rng) % (n - hot);
    } else {
        vpn = rand_r(rng) % n;
    }

    uint64_t off = (uint64_t)rand_r(rng) % (uint64_t)psz;
    return (uint64_t)vpn * (uint64_t)psz + off;
}
