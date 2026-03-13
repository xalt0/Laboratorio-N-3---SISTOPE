#include <stdio.h>
#include <string.h>
#include "../include/simulator.h"
#include "../include/paginacion.h"

config_t         g_cfg;
global_metrics_t g_met;

static int passed = 0, failed = 0;
#define TEST(name, expr) \
    do { if(expr){printf("  [PASS] %s\n",name);passed++;}else{printf("  [FAIL] %s\n",name);failed++;} } while(0)

typedef struct { int id; metrics_t m; } targ_t;
static void *tfunc(void *a) {
    targ_t *t = a; memset(&t->m, 0, sizeof(metrics_t));
    page_run_thread(t->id, &t->m);
    return NULL;
}

static long run(int unsafe) {
    memset(&g_cfg, 0, sizeof(g_cfg)); memset(&g_met, 0, sizeof(g_met));
    pthread_mutex_init(&g_met.lock, NULL);
    g_cfg.mode=MODE_PAGE; g_cfg.num_threads=4; g_cfg.num_pages=32;
    g_cfg.num_frames=8; g_cfg.page_size=4096; g_cfg.tlb_size=8;
    g_cfg.ops_per_thread=100; g_cfg.workload=WL_UNIFORM;
    g_cfg.seed=42; g_cfg.unsafe=unsafe;

    page_system_init(4);
    pthread_t th[4]; targ_t ta[4];
    for(int i=0;i<4;i++){ta[i].id=i; pthread_create(&th[i],NULL,tfunc,&ta[i]);}
    for(int i=0;i<4;i++) pthread_join(th[i],NULL);
    long ops = g_met.total_ops;
    page_system_destroy(4);
    pthread_mutex_destroy(&g_met.lock);
    return ops;
}

int main(void) {
    printf("=== Tests Concurrencia ===\n");

    /* SAFE: todas las ops se registran */
    long safe_ops = run(0);
    TEST("SAFE: total_ops == 4*100", safe_ops == 400);

    /* UNSAFE: no crashea */
    long unsafe_ops = run(1);
    TEST("UNSAFE: no crashea", 1);
    printf("  [INFO] SAFE=%ld, UNSAFE=%ld (puede diferir por carrera)\n",
           safe_ops, unsafe_ops);

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
