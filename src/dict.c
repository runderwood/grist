#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "dict.h"
#include "util.h"

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

int grist_dict_lkup(grist_dict* d, const void* k, size_t ksz, size_t* fidx) {
    size_t idx;
    uint64_t h = d->hash(k, ksz);
    uint64_t p = h;
    size_t j = 0;
    idx = h % d->mask;
    while(j < d->cabsz) {
        if(d->cab[idx] != NULL) {
            if(!d->kcmp(d->cab[idx]->k, d->cab[idx]->ksz, k, ksz)) {
                *fidx = idx;
                return 1;
            }  
        } else {
            *fidx = idx;
            return -1;
        }
        j++;
        idx = ((idx << 2) + idx + p + 1) & d->mask;
        p = p  >> 5;
    }
    return 0;
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
    size_t idx;
    int repl;
    for(i=0; i<oldsz; i++) {
        if(oldtab[i] != NULL) {
            repl = grist_dict_lkup(d, oldtab[i]->k, oldtab[i]->ksz, &idx) > 0 ? 1 : 0;
            grist_dict_put4(d, oldtab[i], idx, repl);
            //grist_dict_entry_del(oldtab[i]);
            //oldtab[i] = NULL;
        }
    }
    free(oldtab);
    return newsz;
}

void grist_dict_del(grist_dict* d) {
    size_t i;
    for(i=0; i<d->cabsz; i++) {
        if(d->cab[i] != NULL) {
            grist_dict_entry_del(d->cab[i]);
            d->cab[i] = NULL;
        }
    }
    free(d->cab);
    free(d);
    return;
}

int grist_dict_put(grist_dict* d, const void *k, size_t ksz, const void* v, size_t vsz, size_t idx, int repl) {
    if(repl) {
        assert(d->cab[idx] != NULL);
        d->cab[idx]->v = realloc(d->cab[idx]->v, vsz);
        assert(d->cab[idx]->v);
        memcpy(d->cab[idx]->v, v, vsz);
        d->cab[idx]->vsz = vsz;
    } else {
        d->cab[idx] = grist_dict_entry_new4(k, ksz, v, vsz);
        d->empty--;
    }
    return 1;
}

int grist_dict_put4(grist_dict* d, grist_dict_entry* e, size_t idx, int repl) {
    if(repl) {
        assert(d->cab[idx] != NULL);
        // reuse del entries here
        grist_dict_entry_del(d->cab[idx]);
        d->cab[idx] = e;
    } else {
        d->cab[idx] = e;
        d->empty--;
    }
    return 1;
}

int grist_dict_set(grist_dict* d, const void* k, size_t ksz, const void* v, size_t vsz) {
    if(((double)d->empty/(double)d->cabsz) < 0.333) {
        grist_dict_resize(d, 8);
    }
    size_t idx;
    int repl = grist_dict_lkup(d, k, ksz, &idx);
    repl = repl == 1 ? 1 : 0;
    int ret = grist_dict_put(d, k, ksz, v, vsz, idx, repl);
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

char* grist_dict_ser(grist_dict* d, size_t* sz) {
    *sz = (d->cabsz-d->empty) * 2 * sizeof(uint64_t);
    size_t i;
    grist_dict_entry* e;
    for(i=0; i<d->cabsz; i++) {
        e = d->cab[i];
        if(!e) continue;
        *sz += e->ksz + e->vsz;
    }
    e = NULL;
    char* ser = malloc(*sz);
    assert(ser);
    size_t offset = 0;
    uint64_t ksz;
    uint64_t vsz;
    for(i=0; i<d->cabsz; i++) {
        e = d->cab[i];
        if(!e) continue;
        ksz = htonll(e->ksz);
        vsz = htonll(e->vsz);
        memcpy(ser+offset, &ksz, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy(ser+offset, &vsz, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy(ser+offset, e->k, e->ksz);
        offset += e->ksz;
        memcpy(ser+offset, e->v, e->vsz);
        offset += e->vsz;
    }
    return ser;
}

grist_dict* grist_dict_unser(const char* buf, size_t bufsz) {
    size_t offset = 0;
    uint64_t ksz;
    uint64_t vsz;
    grist_dict* d = grist_dict_new();
    while(offset < (bufsz-16)) {
        memcpy(&ksz, buf+offset, 8);
        ksz = ntohll(ksz);
        offset += 8;
        memcpy(&vsz, buf+offset, 8);
        vsz = ntohll(vsz);
        offset += 8;
        grist_dict_set(d, buf+offset, ksz, buf+offset+ksz, vsz);
        offset += ksz+vsz;
    }
    return d;
}
