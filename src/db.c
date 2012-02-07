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
    db->hdb = tchdbnew();
    return db;
}

bool grist_db_open(grist_db* db, const char* fname, int omode) {
    return tchdbopen(db->hdb, fname, omode);
}

bool grist_db_close(grist_db* db) {
    return tchdbclose(db->hdb);
}

void grist_db_del(grist_db* db) {
    assert(db);
    tchdbdel(db->hdb);
    free(db);
    db = NULL;
    return;
}

bool grist_db_put(grist_db* db, const void* kbuf, int ksiz, grist_feature* f) {

    int packedsz;
    char* packed = grist_db_packrec(db, f, &packedsz);
    if(!packed) {
        return false;
    }

    if(!tchdbput(db->hdb, kbuf, ksiz, packed, packedsz)) {
        return false;
    }

    return true;
}

grist_feature* grist_db_get(grist_db* db, const void* kbuf, int ksiz) {
    
    int vsz;

    void* v = tchdbget(db->hdb, kbuf, ksiz, &vsz);
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

bool grist_db_iterinit(grist_db* db) {
    return tchdbiterinit(db->hdb);
}

void* grist_db_iternext(grist_db* db, int* szp) {
    return tchdbiternext(db->hdb, szp);
}
