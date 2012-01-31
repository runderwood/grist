#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "db.h"

grist_db* grist_db_new(void) {
    grist_db* db = malloc(sizeof(grist_db));
    db->file = NULL;
    db->curr = -1;
    db->sz = -1;
    db->ver = -1;
    return db;
}

int grist_db_init(grist_db* db) {
    char* hdrbuf = calloc(1, GRIST_DB_HDRSZ);
    assert(hdrbuf);
    uint64_t magic = (uint64_t)(GRIST_DB_MAGIC);
    magic = htonll(magic);
    memcpy(hdrbuf, &magic, 8);
    uint8_t ver = GRIST_DB_VER;
    memcpy(hdrbuf+8, &ver, 1);
    rewind(db->file);
    size_t bw = fwrite(hdrbuf, 1, GRIST_DB_HDRSZ, db->file);
    free(hdrbuf);
    hdrbuf = NULL;
    if(bw == GRIST_DB_HDRSZ)
        return 0;
    return -1;
}

int grist_db_open(grist_db* db, const char* fnm, size_t fnmsz) {
}

int grist_db_close(grist_db* db) {
}

void grist_db_del(grist_db* db) {
    free(db);
    db = NULL;
    return;
}
