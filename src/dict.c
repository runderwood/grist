#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "dict.h"

#include <stdio.h>

uint64_t djb_hash(const void* k, size_t ksz) {
    uint64_t h = 5381;
    size_t i;
    for(i=0; i<ksz; i++) {
        h = ((h << 5) + h) + *(((uint8_t*)k)+i);
    }
    return h;
}

grist_dict_entry* grist_dict_entry_new(void) {
    grist_dict_entry* e = malloc(sizeof(grist_dict_entry));
    return e;
}

grist_dict_entry* grist_dict_entry_new4(const void* k, size_t ksz, const void* v, size_t vsz) {
    grist_dict_entry* e = grist_dict_entry_new();
    e->k = malloc(sizeof(uint8_t)*ksz);
    e->v = malloc(sizeof(uint8_t)*vsz);
    memcpy(e->k, k, ksz);
    memcpy(e->v, v, vsz);
    e->ksz = ksz;
    e->vsz = vsz;
    return e;
}

void grist_dict_entry_del(grist_dict_entry* e) {
    free(e->k);
    free(e->v);
    free(e);
    return;
}

uint8_t grist_dict_kcmp(const void* ka, size_t asz, const void* kb, size_t bsz) {
    assert(asz > 0 && bsz > 0);
    uint8_t cmp = 0;
    size_t i;
    uint8_t a;
    uint8_t b;
    for(i=0; i<asz; i++) {
        a = *(((uint8_t*)ka)+i);
        b = *(((uint8_t*)kb)+i);
        if(a > b) {
            cmp = 1;
        } else if(a < b) {
            cmp = -1;
        }
    }
    if(!cmp && asz != bsz) cmp = asz > bsz ? 1 : -1;
    return cmp;
}

grist_dict* grist_dict_new(void) {
    grist_dict* d = malloc(sizeof(grist_dict));
    d->cabsz = GRIST_DICT_MINSZ;
    d->cab = calloc(d->cabsz, sizeof(grist_dict_entry*));
    d->empty = d->cabsz;
    d->mask = d->cabsz-1;
    d->kcmp = &grist_dict_kcmp;
    d->hash = &djb_hash;
    return d;
}

size_t grist_dict_resize(grist_dict* d, int mult) {
    assert(d->cabsz > 0);
    size_t oldsz = d->cabsz;
    size_t newsz = d->cabsz * mult;
    grist_dict_entry** oldtab = calloc(oldsz, sizeof(grist_dict_entry*));
    size_t i = 0;
    for(i=0; i<oldsz; i++) {
        if(d->cab[i] != NULL) {
            oldtab[i] = d->cab[i];
        }
    }
    free(d->cab);
    d->cab = calloc(newsz, sizeof(grist_dict_entry*));
    d->cabsz = newsz;
    d->empty = newsz;
    d->mask = newsz-1;
    assert(d->cab);
    for(i=0; i<oldsz; i++) {
        if(oldtab[i] != NULL) {
            grist_dict_put(d, oldtab[i]);
            grist_dict_entry_del(oldtab[i]);
            oldtab[i] = NULL;
        }
    }
    free(oldtab);
    printf("resized dict from %zu to %zu.\n", oldsz, newsz);
    return newsz;
}

void grist_dict_del(grist_dict* d) {
    size_t i;
    for(i=0; i<d->cabsz; i++)
        if(d->cab[i] != NULL) 
            grist_dict_entry_del(d->cab[i]);
    free(d->cab);
    free(d);
    return;
}

int grist_dict_put(grist_dict* d, grist_dict_entry* e) {
    size_t idx;
    uint64_t h = d->hash(e->k, e->ksz);
    uint64_t p = h;
    size_t j = 0;
    idx = h % d->mask;
    while(j < d->cabsz) {
        if(d->cab[idx] != NULL) {
            if(!d->kcmp(d->cab[idx]->k, d->cab[idx]->ksz, e->k, e->ksz)) {
                printf("replacing %X\n", *((int*)e->k));
                d->cab[idx]->v = realloc(d->cab[idx]->v, e->vsz);
                assert(d->cab[idx]->v);
                memcpy(d->cab[idx]->v, e->v, e->vsz);
                d->cab[idx]->vsz = e->vsz;
                printf("replaced %zu: %X\n", idx, *((int*)e->k));
                return 1;
            }  
        } else {
            grist_dict_entry* e2 = grist_dict_entry_new4(e->k, e->ksz, e->v, e->vsz);
            d->cab[idx] = e2;
            d->empty--;
            printf("inserted %zu: %X (%zu)\n", idx, *((int*)e->k), d->cabsz - d->empty);
            return 1;
        }
        j++;
        idx = ((idx << 2) + idx + p + 1) & d->mask;
        p = p  >> 5;
    }
    printf("failed.\n");
    return 0;
}

int grist_dict_set(grist_dict* d, const void* k, size_t ksz, void* v, size_t vsz) {
    grist_dict_entry* e = grist_dict_entry_new4(k, ksz, v, vsz);
    if(((double)d->empty/(double)d->cabsz) < 0.333) {
        printf("resize...\n");
        grist_dict_resize(d, 4);
    }
    int ret = grist_dict_put(d, e);
    grist_dict_entry_del(e);
    return ret;
}

void* grist_dict_get(grist_dict* d, const void* sk, size_t sksz, size_t* vsz) {
    uint64_t h = d->hash(sk, sksz);
    uint64_t p = h;
    size_t idx = h % d->mask;
    size_t j = 0;
    while(j < d->cabsz) {
        grist_dict_entry* e = d->cab[idx];
        if(e == NULL) return NULL;
        if(!d->kcmp(sk, sksz, e->k, e->ksz)) {
            *vsz = e->vsz;
            void* v = malloc(*vsz);
            memcpy(v, e->v, *vsz);
            return v;
        }
        // macro here.
        idx = ((idx << 2) + idx + p + 1) & d->mask;
        p = p  >> 5;
        j++;
    }
    return NULL;
}


