#ifndef GRIST_DICT_H
#define GRIST_DICT_H

#include <stdint.h>

#define GRIST_DICT_MINSZ 8

typedef struct grist_dict_entry_s {
    void* k;
    size_t ksz;
    void* v;
    size_t vsz;
} grist_dict_entry;

typedef uint8_t grist_kcmp_fxn(const void *ka, size_t asz, const void* kb, size_t bsz);
typedef uint64_t grist_khash_fxn(const void* k, size_t ksz);

typedef struct grist_dict_s {
    size_t cabsz;
    size_t mask;
    size_t empty;
    grist_dict_entry** cab;
    grist_kcmp_fxn* kcmp;
    grist_khash_fxn* hash; 
} grist_dict;

uint64_t djb_hash(const void* k, const size_t ksz);
grist_dict_entry* grist_dict_entry_new(void);
grist_dict_entry* grist_dict_entry_new4(const void* k, size_t ksz, const void* v, size_t vsz);
void grist_dict_entry_del(grist_dict_entry* e);
grist_dict* grist_dict_new(void);
void grist_dict_del(grist_dict* d);
int grist_dict_put(grist_dict* d, const void* k, size_t ksz, const void* v, size_t vsz, size_t idx, int repl);
int grist_dict_put4(grist_dict* d, grist_dict_entry* e, size_t idx, int repl);
void* grist_dict_get(grist_dict* d, const void* k, size_t ksz, size_t* vsz);
int grist_dict_set(grist_dict* d, const void* k, size_t ksz, const void* v, size_t vsz);
size_t grist_dict_resize(grist_dict* d, int mult);

char* grist_dict_ser(grist_dict* d, size_t* sz);
grist_dict* grist_dict_unser(const char* buf, size_t bufsz);

#endif
