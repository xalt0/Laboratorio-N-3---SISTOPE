#include "segmentacion.h"
#include "workloads.h"

/* Inicializa la tabla de segmentos del thread.
 * Cada segmento recibe una base contigua, separada por thread. */
void seg_init(segment_table_t *st, int thread_id) {
    int n = g_cfg.num_segments;
    st->num_segments = n;
    st->segments = malloc((size_t)n * sizeof(segment_entry_t));

    uint64_t total = 0;
    for (int i = 0; i < n; i++) total += g_cfg.seg_limits[i];

    uint64_t base = (uint64_t)thread_id * total;
    for (int i = 0; i < n; i++) {
        st->segments[i].base  = base;
        st->segments[i].limit = g_cfg.seg_limits[i];
        base += g_cfg.seg_limits[i];
    }
}

void seg_destroy(segment_table_t *st) {
    free(st->segments);
    st->segments = NULL;
}

/* Traduce (seg_id, offset) → PA.
 * Retorna 0 si OK, -1 si segfault (offset >= limit o seg_id inválido). */
int seg_translate(segment_table_t *st, int seg_id, uint64_t offset,
                  uint64_t *pa_out, metrics_t *m) {
    if (seg_id < 0 || seg_id >= st->num_segments ||
        offset >= st->segments[seg_id].limit) {
        m->segfaults++;
        return -1;
    }
    *pa_out = st->segments[seg_id].base + offset;
    m->translations_ok++;
    return 0;
}

/* Ejecuta todas las operaciones del thread en modo segmentación. */
void seg_run_thread(int thread_id, metrics_t *m) {
    segment_table_t st;
    seg_init(&st, thread_id);

    unsigned rng = g_cfg.seed + (unsigned)thread_id;
    double total = 0.0;

    for (int i = 0; i < g_cfg.ops_per_thread; i++) {
        int seg_id; uint64_t offset;
        wl_gen_seg(&rng, &seg_id, &offset);

        double t0 = now_ns();
        uint64_t pa;
        seg_translate(&st, seg_id, offset, &pa, m);
        total += now_ns() - t0;
        m->total_ops++;
    }

    m->total_time_ns = total;

    /* Acumular en métricas globales */
    if (!g_cfg.unsafe) pthread_mutex_lock(&g_met.lock);
    g_met.translations_ok += m->translations_ok;
    g_met.segfaults        += m->segfaults;
    g_met.total_ops        += m->total_ops;
    if (!g_cfg.unsafe) pthread_mutex_unlock(&g_met.lock);

    seg_destroy(&st);
}
