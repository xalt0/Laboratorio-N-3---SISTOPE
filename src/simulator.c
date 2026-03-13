#include "simulator.h"
#include "segmentacion.h"
#include "paginacion.h"
#include <getopt.h>
#include <sys/stat.h>

/* ── Variables globales ────────────────────────────────── */
config_t        g_cfg;
global_metrics_t g_met;

/* ── Argumento de cada thread ──────────────────────────── */
typedef struct {
    int       thread_id;
    metrics_t m;
} targ_t;

static void *thread_func(void *arg) {
    targ_t *t = (targ_t *)arg;
    memset(&t->m, 0, sizeof(metrics_t));
    if (g_cfg.mode == MODE_SEG)
        seg_run_thread(t->thread_id, &t->m);
    else
        page_run_thread(t->thread_id, &t->m);
    return NULL;
}

/* ── Parseo de argumentos ──────────────────────────────── */
static void parse_args(int argc, char **argv) {
    g_cfg.mode           = MODE_SEG;
    g_cfg.num_threads    = 1;
    g_cfg.ops_per_thread = 1000;
    g_cfg.workload       = WL_UNIFORM;
    g_cfg.seed           = 42;
    g_cfg.unsafe         = 0;
    g_cfg.show_stats     = 0;
    g_cfg.num_segments   = 4;
    g_cfg.num_pages      = 64;
    g_cfg.num_frames     = 32;
    g_cfg.page_size      = 4096;
    g_cfg.tlb_size       = 16;
    for (int i = 0; i < 64; i++) g_cfg.seg_limits[i] = 4096;

    static struct option opts[] = {
        {"mode",           required_argument, 0, 'm'},
        {"threads",        required_argument, 0, 't'},
        {"ops-per-thread", required_argument, 0, 'o'},
        {"workload",       required_argument, 0, 'w'},
        {"seed",           required_argument, 0, 'S'},
        {"unsafe",         no_argument,       0, 'u'},
        {"stats",          no_argument,       0, 's'},
        {"segments",       required_argument, 0, 'g'},
        {"seg-limits",     required_argument, 0, 'l'},
        {"pages",          required_argument, 0, 'p'},
        {"frames",         required_argument, 0, 'f'},
        {"page-size",      required_argument, 0, 'z'},
        {"tlb-size",       required_argument, 0, 'T'},
        {"tlb-policy",     required_argument, 0, 'P'},
        {"evict-policy",   required_argument, 0, 'E'},
        {0, 0, 0, 0}
    };

    int opt, idx;
    while ((opt = getopt_long(argc, argv, "", opts, &idx)) != -1) {
        switch (opt) {
        case 'm':
            if      (strcmp(optarg,"seg")  == 0) g_cfg.mode = MODE_SEG;
            else if (strcmp(optarg,"page") == 0) g_cfg.mode = MODE_PAGE;
            break;
        case 't': g_cfg.num_threads    = atoi(optarg); break;
        case 'o': g_cfg.ops_per_thread = atoi(optarg); break;
        case 'w':
            if (strcmp(optarg,"80-20") == 0) g_cfg.workload = WL_8020;
            else                             g_cfg.workload = WL_UNIFORM;
            break;
        case 'S': g_cfg.seed    = (unsigned)atoi(optarg); break;
        case 'u': g_cfg.unsafe  = 1; break;
        case 's': g_cfg.show_stats = 1; break;
        case 'g':
            g_cfg.num_segments = atoi(optarg);
            break;
        case 'l': {
            char *buf = strdup(optarg), *tok = strtok(buf, ",");
            for (int i = 0; tok && i < 64; i++, tok = strtok(NULL, ","))
                g_cfg.seg_limits[i] = (uint64_t)atol(tok);
            free(buf);
            break;
        }
        case 'p': g_cfg.num_pages  = atoi(optarg); break;
        case 'f': g_cfg.num_frames = atoi(optarg); break;
        case 'z': g_cfg.page_size  = atoi(optarg); break;
        case 'T': g_cfg.tlb_size   = atoi(optarg); break;
        case 'P': case 'E': break; /* Solo FIFO disponible */
        }
    }
}

/* ── Reporte en consola (--stats) ──────────────────────── */
static void print_stats(targ_t *targs, int n, double wall_sec) {
    printf("\n========================================\n");
    printf("SIMULADOR DE MEMORIA VIRTUAL\n");
    printf("Modo: %s\n", g_cfg.mode == MODE_SEG ? "SEGMENTACIÓN" : "PAGINACIÓN");
    printf("========================================\n");

    printf("\nConfiguración:\n");
    printf("  Threads         : %d\n", g_cfg.num_threads);
    printf("  Ops por thread  : %d\n", g_cfg.ops_per_thread);
    printf("  Workload        : %s\n", g_cfg.workload == WL_UNIFORM ? "uniform" : "80-20");
    printf("  Seed            : %u\n", g_cfg.seed);

    if (g_cfg.mode == MODE_SEG) {
        printf("  Segmentos       : %d\n", g_cfg.num_segments);
        printf("  Límites         : ");
        for (int i = 0; i < g_cfg.num_segments; i++)
            printf("%lu%s", g_cfg.seg_limits[i], i < g_cfg.num_segments-1 ? "," : "\n");
    } else {
        printf("  Páginas         : %d\n", g_cfg.num_pages);
        printf("  Frames          : %d\n", g_cfg.num_frames);
        printf("  Tamaño página   : %d bytes\n", g_cfg.page_size);
        printf("  TLB size        : %d\n", g_cfg.tlb_size);
    }

    printf("\nMétricas Globales:\n");
    if (g_cfg.mode == MODE_SEG) {
        printf("  translations_ok     : %ld\n", g_met.translations_ok);
        printf("  segfaults           : %ld\n", g_met.segfaults);
    } else {
        long tot = g_met.tlb_hits + g_met.tlb_misses;
        double hr = tot > 0 ? (double)g_met.tlb_hits / tot * 100.0 : 0.0;
        printf("  tlb_hits            : %ld\n", g_met.tlb_hits);
        printf("  tlb_misses          : %ld\n", g_met.tlb_misses);
        printf("  hit_rate            : %.2f%%\n", hr);
        printf("  page_faults         : %ld\n", g_met.page_faults);
        printf("  evictions           : %ld\n", g_met.evictions);
    }

    double avg_ns = 0.0;
    for (int i = 0; i < n; i++)
        avg_ns += targs[i].m.total_time_ns;
    avg_ns /= (double)g_met.total_ops;
    printf("  avg_translation_ns  : %.2f\n", avg_ns);

    double throughput = wall_sec > 0 ? (double)g_met.total_ops / wall_sec : 0.0;

    printf("\nMétricas por Thread:\n");
    for (int i = 0; i < n; i++) {
        metrics_t *m = &targs[i].m;
        double avg = m->total_ops > 0 ? m->total_time_ns / m->total_ops : 0.0;
        printf("  Thread %d: ", i);
        if (g_cfg.mode == MODE_SEG)
            printf("ok=%ld  segfaults=%ld  avg_ns=%.2f\n",
                   m->translations_ok, m->segfaults, avg);
        else
            printf("tlb_hits=%ld  tlb_misses=%ld  page_faults=%ld  evictions=%ld  avg_ns=%.2f\n",
                   m->tlb_hits, m->tlb_misses, m->page_faults, m->evictions, avg);
    }

    printf("\nTiempo total    : %.3f segundos\n", wall_sec);
    printf("Throughput      : %.2f ops/seg\n", throughput);
    printf("========================================\n\n");
}

/* ── Generar out/summary.json ──────────────────────────── */
static void write_json(targ_t *targs, int n, double wall_sec) {
    mkdir("out", 0755);
    FILE *f = fopen("out/summary.json", "w");
    if (!f) { perror("fopen"); return; }

    long tot_tlb = g_met.tlb_hits + g_met.tlb_misses;
    double hr    = tot_tlb > 0 ? (double)g_met.tlb_hits / tot_tlb : 0.0;
    double avg_ns = 0.0;
    for (int i = 0; i < n; i++) avg_ns += targs[i].m.total_time_ns;
    avg_ns /= g_met.total_ops > 0 ? (double)g_met.total_ops : 1.0;
    double tp = wall_sec > 0 ? (double)g_met.total_ops / wall_sec : 0.0;

    fprintf(f, "{\n  \"mode\": \"%s\",\n",
            g_cfg.mode == MODE_SEG ? "seg" : "page");
    fprintf(f, "  \"config\": {\n");
    fprintf(f, "    \"threads\": %d,\n", g_cfg.num_threads);
    fprintf(f, "    \"ops_per_thread\": %d,\n", g_cfg.ops_per_thread);
    fprintf(f, "    \"workload\": \"%s\",\n",
            g_cfg.workload == WL_UNIFORM ? "uniform" : "80-20");
    fprintf(f, "    \"seed\": %u,\n", g_cfg.seed);
    fprintf(f, "    \"unsafe\": %s", g_cfg.unsafe ? "true" : "false");
    if (g_cfg.mode == MODE_SEG) {
        fprintf(f, ",\n    \"segments\": %d,\n", g_cfg.num_segments);
        fprintf(f, "    \"seg_limits\": [");
        for (int i = 0; i < g_cfg.num_segments; i++)
            fprintf(f, "%lu%s", g_cfg.seg_limits[i],
                    i < g_cfg.num_segments-1 ? "," : "");
        fprintf(f, "]\n");
    } else {
        fprintf(f, ",\n    \"pages\": %d,\n", g_cfg.num_pages);
        fprintf(f, "    \"frames\": %d,\n", g_cfg.num_frames);
        fprintf(f, "    \"page_size\": %d,\n", g_cfg.page_size);
        fprintf(f, "    \"tlb_size\": %d,\n", g_cfg.tlb_size);
        fprintf(f, "    \"tlb_policy\": \"fifo\",\n");
        fprintf(f, "    \"evict_policy\": \"fifo\"\n");
    }
    fprintf(f, "  },\n  \"metrics\": {\n");
    if (g_cfg.mode == MODE_SEG) {
        fprintf(f, "    \"translations_ok\": %ld,\n", g_met.translations_ok);
        fprintf(f, "    \"segfaults\": %ld,\n", g_met.segfaults);
    } else {
        fprintf(f, "    \"tlb_hits\": %ld,\n", g_met.tlb_hits);
        fprintf(f, "    \"tlb_misses\": %ld,\n", g_met.tlb_misses);
        fprintf(f, "    \"hit_rate\": %.4f,\n", hr);
        fprintf(f, "    \"page_faults\": %ld,\n", g_met.page_faults);
        fprintf(f, "    \"evictions\": %ld,\n", g_met.evictions);
    }
    fprintf(f, "    \"avg_translation_time_ns\": %.2f,\n", avg_ns);
    fprintf(f, "    \"throughput_ops_sec\": %.2f\n", tp);
    fprintf(f, "  },\n  \"runtime_sec\": %.4f\n}\n", wall_sec);
    fclose(f);
}

/* ── main ──────────────────────────────────────────────── */
int main(int argc, char **argv) {
    parse_args(argc, argv);
    srand(g_cfg.seed);

    memset(&g_met, 0, sizeof(global_metrics_t));
    pthread_mutex_init(&g_met.lock, NULL);

    if (g_cfg.mode == MODE_PAGE)
        page_system_init(g_cfg.num_threads);

    pthread_t *threads = malloc((size_t)g_cfg.num_threads * sizeof(pthread_t));
    targ_t    *targs   = malloc((size_t)g_cfg.num_threads * sizeof(targ_t));

    double t0 = now_ns();
    for (int i = 0; i < g_cfg.num_threads; i++) {
        targs[i].thread_id = i;
        pthread_create(&threads[i], NULL, thread_func, &targs[i]);
    }
    for (int i = 0; i < g_cfg.num_threads; i++)
        pthread_join(threads[i], NULL);
    double wall = (now_ns() - t0) / 1e9;

    if (g_cfg.show_stats)
        print_stats(targs, g_cfg.num_threads, wall);

    write_json(targs, g_cfg.num_threads, wall);

    if (g_cfg.mode == MODE_PAGE)
        page_system_destroy(g_cfg.num_threads);

    pthread_mutex_destroy(&g_met.lock);
    free(threads);
    free(targs);
    return 0;
}
