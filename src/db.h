#ifndef GRIST_DB_H
#define GRIST_DB_H

#include <stdio.h>
#include <stdint.h>
#include "util.h"

#define GRIST_DB_HDRSZ 128
#define GRIST_DB_VER 0x01
#define GRIST_DB_MAGIC 0x4752495354404040
#define GRIST_DB_MAGICSZ 8

typedef struct grist_db_s {
    FILE* file;
    uint32_t curr;
    uint32_t sz;
    uint8_t ver;
} grist_db;

grist_db* grist_db_new(void);
int grist_db_open(grist_db* db, const char* fnm, size_t fnmsz);
int grist_db_close(grist_db* db);
void grist_db_del(grist_db* db);

uint8_t grist_db_ver(grist_db* db);
int grist_db_init(grist_db* db);

#endif
