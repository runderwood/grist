#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <json/json.h>
#include "db.h"
#include "feature.h"

grist_db* grist_db_new(void) {
    grist_db* db = malloc(sizeof(grist_db));
    db->bdb = tcbdbnew();
    return db;
}

bool grist_db_open(grist_db* db, const char* fname, int omode) {
    assert(tcbdbtune(db->bdb, -1, -1, -1, -1, -1, BDBTLARGE|BDBTBZIP));
    return tcbdbopen(db->bdb, fname, omode);
}

bool grist_db_close(grist_db* db) {
    return tcbdbclose(db->bdb);
}

void grist_db_del(grist_db* db) {
    assert(db);
    tcbdbdel(db->bdb);
    free(db);
    db = NULL;
    return;
}

bool grist_db_put(grist_db* db, const void* kbuf, int ksiz, grist_feature* f) {

    int packedsz = 0;
    char* packed = grist_db_packrec(db, f, &packedsz);
    if(!packed || !packedsz) {
        return false;
    }

    if(!tcbdbput(db->bdb, kbuf, ksiz, packed, packedsz)) {
        return false;
    }

    return true;
}

grist_feature* grist_db_get(grist_db* db, const void* kbuf, int ksiz) {
    
    int vsz;

    void* v = tcbdbget(db->bdb, kbuf, ksiz, &vsz);
    if(!v) {
        return NULL;
    }

    grist_feature* f = grist_db_unpackrec(db, v, vsz);

    free(v);
    return f;
}

void* grist_db_packrec(grist_db* db, grist_feature* f, int* sz) {
    return grist_feature_pack(f, sz);
}

grist_feature* grist_db_unpackrec(grist_db* db, void* v, int vsz) {
    return grist_feature_unpack(v, vsz);
}

BDBCUR* grist_db_curnew(grist_db* db) {
    BDBCUR* cur = tcbdbcurnew(db->bdb);
    assert(cur);
    tcbdbcurfirst(cur);
    return cur;
}

bool grist_db_curnext(BDBCUR* cur) {
    return tcbdbcurnext(cur);
}

void* grist_db_curkey(BDBCUR* cur, int* szp) {
    return tcbdbcurkey(cur, szp);
}

uint64_t grist_db_fcount(grist_db* db) {
    return tcbdbrnum(db->bdb);
}

uint64_t grist_db_filesz(grist_db* db) {
    return tcbdbfsiz(db->bdb);
}

const char* grist_db_errmsg(grist_db* db) {
    int ecode = tcbdbecode(db->bdb);
    if(!ecode) return NULL;
    return tcbdberrmsg(ecode);
}
