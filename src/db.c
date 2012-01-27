#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
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
    char* fnm_ = malloc(fnmsz+1);
    memcpy(fnm_, fnm, fnmsz);
    fnm_[fnmsz] = '\0';
    int init = 0;
    if(access(fnm_, W_OK)) init = 1;
    db->file = fopen(fnm_, "w+"); // be smarter about this...fcntl, etc.
    free(fnm_);
    fnm_ = NULL;
    if(!db->file) return -1;
    db->curr = 0;
    if(init) {
        printf("init\n");
        return grist_db_init(db);
    }
    return 0;
}

int grist_db_close(grist_db* db) {
    return fclose(db->file) == EOF;
}

void grist_db_del(grist_db* db) {
    free(db);
    db = NULL;
    return;
}
