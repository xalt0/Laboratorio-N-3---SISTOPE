#include "tlb.h"

void tlb_init(tlb_t *tlb, int size) {
    tlb->size       = size;
    tlb->next_index = 0;
    tlb->count      = 0;
    tlb->entries    = (size > 0) ? calloc((size_t)size, sizeof(tlb_entry_t)) : NULL;
}

void tlb_destroy(tlb_t *tlb) {
    free(tlb->entries);
    tlb->entries = NULL;
}

/* Retorna 1 (hit) y escribe el frame, o 0 (miss). */
int tlb_lookup(tlb_t *tlb, uint64_t vpn, uint64_t *frame_out) {
    if (tlb->size == 0 || tlb->count == 0) return 0;
    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].vpn == vpn) {
            *frame_out = tlb->entries[i].frame_number;
            return 1;
        }
    }
    return 0;
}

/* Inserta con política FIFO. Si ya existe el VPN, actualiza en su lugar. */
void tlb_insert(tlb_t *tlb, uint64_t vpn, uint64_t frame_number) {
    if (tlb->size == 0) return;
    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].vpn == vpn) {
            tlb->entries[i].frame_number = frame_number;
            return;
        }
    }
    tlb->entries[tlb->next_index].vpn          = vpn;
    tlb->entries[tlb->next_index].frame_number = frame_number;
    tlb->entries[tlb->next_index].valid        = 1;
    tlb->next_index = (tlb->next_index + 1) % tlb->size;
    if (tlb->count < tlb->size) tlb->count++;
}

/* Invalida todas las entradas con ese VPN (llamado tras eviction). */
void tlb_invalidate(tlb_t *tlb, uint64_t vpn) {
    if (tlb->size == 0) return;
    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].vpn == vpn) {
            tlb->entries[i].valid = 0;
            if (tlb->count > 0) tlb->count--;
        }
    }
}
