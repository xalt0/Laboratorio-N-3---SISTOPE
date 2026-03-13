#include <stdio.h>
#include <string.h>
#include "../include/simulator.h"
#include "../include/segmentacion.h"

config_t         g_cfg;
global_metrics_t g_met;

static int passed = 0, failed = 0;
#define TEST(name, expr) \
    do { if(expr){printf("  [PASS] %s\n",name);passed++;}else{printf("  [FAIL] %s\n",name);failed++;} } while(0)

int main(void) {
    printf("=== Tests Segmentación ===\n");

    /* Setup */
    memset(&g_cfg, 0, sizeof(g_cfg));
    pthread_mutex_init(&g_met.lock, NULL);
    g_cfg.mode = MODE_SEG; g_cfg.num_segments = 4; g_cfg.unsafe = 1;
    uint64_t lims[] = {1024,2048,4096,8192};
    for(int i=0;i<4;i++) g_cfg.seg_limits[i] = lims[i];

    segment_table_t st;
    seg_init(&st, 0);
    metrics_t m = {0};
    uint64_t pa;

    /* 1. Traducción correcta */
    int r = seg_translate(&st, 0, 100, &pa, &m);
    TEST("traduccion correcta retorna 0", r == 0);
    TEST("PA = base + offset", pa == st.segments[0].base + 100);
    TEST("translations_ok incrementa", m.translations_ok == 1);

    /* 2. Segfault por offset >= limit */
    r = seg_translate(&st, 0, 1024, &pa, &m);
    TEST("offset==limit genera segfault", r == -1);
    TEST("segfaults incrementa", m.segfaults == 1);

    /* 3. translations_ok + segfaults == ops totales */
    g_cfg.ops_per_thread = 1000; g_cfg.workload = WL_UNIFORM; g_cfg.seed = 42;
    metrics_t m2 = {0};
    seg_run_thread(0, &m2);
    TEST("ok + segfaults == ops_per_thread", m2.translations_ok + m2.segfaults == 1000);

    seg_destroy(&st);
    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
