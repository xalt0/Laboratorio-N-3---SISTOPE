#include "frame_allocator.h"
#include "paginacion.h"

void fa_init(frame_allocator_t *fa, int num_frames) {
    fa->num_frames = num_frames;
    fa->head       = NULL;
    fa->tail       = NULL;
    fa->free_list  = malloc((size_t)num_frames * sizeof(int));
    for (int i = 0; i < num_frames; i++) fa->free_list[i] = 1;
    pthread_mutex_init(&fa->lock, NULL);
}

void fa_destroy(frame_allocator_t *fa) {
    free(fa->free_list);
    fifo_node_t *cur = fa->head;
    while (cur) { fifo_node_t *nx = cur->next; free(cur); cur = nx; }
    pthread_mutex_destroy(&fa->lock);
}

static int find_free(frame_allocator_t *fa) {
    for (int i = 0; i < fa->num_frames; i++)
        if (fa->free_list[i]) { fa->free_list[i] = 0; return i; }
    return -1;
}

static void enqueue(frame_allocator_t *fa, uint64_t frame, uint64_t vpn, int tid) {
    fifo_node_t *n = malloc(sizeof(fifo_node_t));
    n->frame = frame; n->vpn = vpn; n->thread_id = tid; n->next = NULL;
    if (fa->tail) fa->tail->next = n; else fa->head = n;
    fa->tail = n;
}

/* Evicta la página más antigua (FIFO).
 * Invalida en la tabla de páginas del dueño y en todas las TLBs. */
static uint64_t evict(frame_allocator_t *fa,
                      uint64_t *evicted_vpn, int *evicted_tid) {
    fifo_node_t *v = fa->head;
    fa->head = v->next;
    if (!fa->head) fa->tail = NULL;

    uint64_t frame = v->frame;
    *evicted_vpn   = v->vpn;
    *evicted_tid   = v->thread_id;

    /* Invalida en tabla de páginas del dueño */
    page_state_t *owner = &g_page_states[v->thread_id];
    if ((int)v->vpn < owner->pt.num_pages) {
        owner->pt.entries[v->vpn].valid        = 0;
        owner->pt.entries[v->vpn].frame_number = INVALID_FRAME;
    }
    /* Invalida en todas las TLBs */
    for (int i = 0; i < g_cfg.num_threads; i++)
        tlb_invalidate(&g_page_states[i].tlb, v->vpn);

    free(v);
    return frame;
}

/* Asigna un frame. Si no hay libres, evicta FIFO.
 * Protegido por mutex en modo SAFE. */
uint64_t fa_alloc(frame_allocator_t *fa, uint64_t vpn, int tid,
                  uint64_t *evicted_vpn, int *evicted_tid, int *did_evict) {
    if (!g_cfg.unsafe) pthread_mutex_lock(&fa->lock);

    *did_evict = 0;
    int f = find_free(fa);
    uint64_t frame;

    if (f >= 0) {
        frame = (uint64_t)f;
    } else {
        frame = evict(fa, evicted_vpn, evicted_tid);
        *did_evict = 1;
    }
    enqueue(fa, frame, vpn, tid);

    if (!g_cfg.unsafe) pthread_mutex_unlock(&fa->lock);
    return frame;
}
