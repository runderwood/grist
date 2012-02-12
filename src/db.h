#ifndef GRIST_DB_H
#define GRIST_DB_H

#include <tcbdb.h>
#include "util.h"
#include "feature.h"

#define GRIST_DB_HDRSZ 128
#define GRIST_DB_VER 0x01
#define GRIST_DB_TYP 0x01
#define GRIST_DB_MAGIC "GrIsT@@@"
#define GRIST_DB_MAGICSZ 8

typedef struct grist_db_s {
    TCBDB* bdb;
} grist_db;

grist_db* grist_db_new(void);
bool grist_db_open(grist_db* db, const char* fname, int omode);
bool grist_db_close(grist_db* db);
void grist_db_del(grist_db* db);

void* grist_db_packrec(grist_db* db, grist_feature* f, int* sz);
grist_feature* grist_db_unpackrec(grist_db* db, void* v, int vsz);
bool grist_db_put(grist_db* db, const void* kbuf, int ksiz, grist_feature* f);
grist_feature* grist_db_get(grist_db* db, const void *kbuf, int ksz);

BDBCUR* grist_db_curnew(grist_db* db);
bool grist_db_curnext(BDBCUR* cur);
void* grist_db_curkey(BDBCUR* cur, int* ksz);

uint64_t grist_db_fcount(grist_db* db);
uint64_t grist_db_filesz(grist_db* db);

const char* grist_db_errmsg(grist_db* db);

#endif
