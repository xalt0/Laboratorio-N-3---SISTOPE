#include "paginacion.h"
#include "workloads.h"

frame_allocator_t  g_fa;
page_state_t      *g_page_states = NULL;

void page_system_init(int num_threads) {
    fa_init(&g_fa, g_cfg.num_frames);
    g_page_states = malloc((size_t)num_threads * sizeof(page_state_t));

    for (int t = 0; t < num_threads; t++) {
        page_state_t *s = &g_page_states[t];
        s->thread_id    = t;
        s->pt.num_pages = g_cfg.num_pages;
        s->pt.entries   = calloc((size_t)g_cfg.num_pages, sizeof(page_table_entry_t));
        for (int p = 0; p < g_cfg.num_pages; p++) {
            s->pt.entries[p].valid        = 0;
            s->pt.entries[p].frame_number = INVALID_FRAME;
        }
        tlb_init(&s->tlb, g_cfg.tlb_size);
    }
}

void page_system_destroy(int num_threads) {
    for (int t = 0; t < num_threads; t++) {
        free(g_page_states[t].pt.entries);
        tlb_destroy(&g_page_states[t].tlb);
    }
    free(g_page_states);
    g_page_states = NULL;
    fa_destroy(&g_fa);
}

/* Simula el delay de carga desde disco (1–5 ms). */
static void disk_delay(void) {
    struct timespec ts = { 0, 1000000L + (rand() % 4000000L) };
    nanosleep(&ts, NULL);
}

/* Traduce VA → PA para el thread dado.
 * Flujo: TLB → tabla de páginas → page fault si no está en memoria. */
static void page_translate(page_state_t *s, uint64_t va, metrics_t *m) {
    uint64_t psz    = (uint64_t)g_cfg.page_size;
    uint64_t vpn    = va / psz;
    uint64_t offset = va % psz;
    uint64_t frame;

    /* 1. Consultar TLB */
    if (tlb_lookup(&s->tlb, vpn, &frame)) {
        m->tlb_hits++;
    } else {
        m->tlb_misses++;

        /* 2. Consultar tabla de páginas */
        page_table_entry_t *pte = &s->pt.entries[vpn];
        if (pte->valid) {
            frame = pte->frame_number;
            tlb_insert(&s->tlb, vpn, frame);
        } else {
            /* 3. Page fault */
            m->page_faults++;
            uint64_t evicted_vpn; int evicted_tid, did_evict;
            frame = fa_alloc(&g_fa, vpn, s->thread_id,
                             &evicted_vpn, &evicted_tid, &did_evict);
            if (did_evict) m->evictions++;

            disk_delay();  /* Simular carga desde disco */

            pte->valid        = 1;
            pte->frame_number = frame;
            tlb_insert(&s->tlb, vpn, frame);
        }
    }
    /* PA = frame * page_size + offset (no se usa el valor, solo se verifica) */
    (void)(frame * psz + offset);
}

/* Ejecuta todas las operaciones del thread en modo paginación. */
void page_run_thread(int thread_id, metrics_t *m) {
    page_state_t *s   = &g_page_states[thread_id];
    unsigned      rng = g_cfg.seed + (unsigned)thread_id;
    double        total = 0.0;

    for (int i = 0; i < g_cfg.ops_per_thread; i++) {
        uint64_t va = wl_gen_page(&rng);
        double t0 = now_ns();
        page_translate(s, va, m);
        total += now_ns() - t0;
        m->total_ops++;
    }

    m->total_time_ns = total;

    /* Acumular en métricas globales */
    if (!g_cfg.unsafe) pthread_mutex_lock(&g_met.lock);
    g_met.tlb_hits    += m->tlb_hits;
    g_met.tlb_misses  += m->tlb_misses;
    g_met.page_faults += m->page_faults;
    g_met.evictions   += m->evictions;
    g_met.total_ops   += m->total_ops;
    if (!g_cfg.unsafe) pthread_mutex_unlock(&g_met.lock);
}
