#include <stdio.h>
#include <string.h>
#include "../include/simulator.h"
#include "../include/tlb.h"
#include "../include/paginacion.h"

config_t         g_cfg;
global_metrics_t g_met;

static int passed = 0, failed = 0;
#define TEST(name, expr) \
    do { if(expr){printf("  [PASS] %s\n",name);passed++;}else{printf("  [FAIL] %s\n",name);failed++;} } while(0)

int main(void) {
    printf("=== Tests Paginación y TLB ===\n");
    memset(&g_cfg, 0, sizeof(g_cfg));
    pthread_mutex_init(&g_met.lock, NULL);
    g_cfg.mode = MODE_PAGE; g_cfg.unsafe = 1;
    g_cfg.num_threads = 1; g_cfg.num_pages = 8;
    g_cfg.num_frames = 4; g_cfg.page_size = 4096; g_cfg.tlb_size = 4;

    /* TLB: lookup, insert, FIFO, invalidate */
    tlb_t tlb; tlb_init(&tlb, 2);
    uint64_t frame;
    TEST("tlb vacía → miss", tlb_lookup(&tlb, 5, &frame) == 0);
    tlb_insert(&tlb, 5, 10);
    TEST("insert y lookup → hit", tlb_lookup(&tlb, 5, &frame) == 1 && frame == 10);
    tlb_insert(&tlb, 6, 20);  /* llena */
    tlb_insert(&tlb, 7, 30);  /* desaloja vpn=5 (FIFO) */
    TEST("FIFO: vpn=5 desalojado", tlb_lookup(&tlb, 5, &frame) == 0);
    tlb_insert(&tlb, 8, 40);
    tlb_invalidate(&tlb, 6);
    TEST("invalidate vpn=6 → miss", tlb_lookup(&tlb, 6, &frame) == 0);
    tlb_destroy(&tlb);

    /* Descomposición VA → VPN + offset */
    uint64_t va = 3 * 4096 + 512;
    TEST("VPN = 3",     va / 4096 == 3);
    TEST("offset = 512", va % 4096 == 512);

    /* Page fault en primer acceso */
    g_cfg.ops_per_thread = 0;
    page_system_init(1);
    /* Acceder directamente a VPN=0 usando page_run_thread con 1 op */
    g_cfg.ops_per_thread = 1;
    metrics_t m = {0};
    page_run_thread(0, &m);
    TEST("primer acceso genera page_fault", m.page_faults >= 1);
    page_system_destroy(1);

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
