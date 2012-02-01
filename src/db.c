#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
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

    uint64_t pgsz;
    memcpy(&pgsz, v, sizeof(uint64_t));
    uint64_t mdsz;
    memcpy(&mdsz, v+sizeof(uint64_t), sizeof(uint64_t));

    pgsz = ntohll(pgsz);
    mdsz = ntohll(mdsz);

    GEOSWKBReader* r = GEOSWKBReader_create();

    GEOSGeometry* g = GEOSWKBReader_read(r, v+(2*sizeof(uint64_t)), pgsz);
    GEOSWKBReader_destroy(r);

    if(!g) {
        free(v);
        return NULL;
    }

    TCMAP* map = tcmapload(v+(2*sizeof(uint64_t))+pgsz, mdsz);
    if(!map) {
        free(v);
        GEOSGeom_destroy(g);
        return NULL;
    }
    
    grist_feature* f = grist_feature_new();
    f->geom = g;
    f->attr = map;

    return f;
}

void* grist_db_packrec(grist_db* db, grist_feature* f, int* sz) {
    char* packed = NULL;

    GEOSWKBWriter* w = GEOSWKBWriter_create();
    GEOSWKBWriter_setByteOrder(w, GEOS_WKB_XDR);

    size_t pgsz;
    unsigned char* wkb = GEOSWKBWriter_write(w, f->geom, &pgsz);
    if(!wkb || !pgsz) {
        return packed;
    }

    int mdsz;
    char* mapdump = tcmapdump(f->attr, &mdsz);
    if(!mapdump || !mdsz) {
        return packed;
    }

    packed = malloc(*sz);
    if(!packed) {
        return packed;
    }

    uint64_t pgsz_ = htonll((uint64_t)pgsz);
    uint64_t mdsz_ = htonll((uint64_t)mdsz);

    size_t offset = 0;
    memcpy(packed, &pgsz_, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    memcpy(packed+offset, &mdsz_, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    memcpy(packed+offset, wkb, pgsz);
    offset += pgsz;
    memcpy(packed+offset, mapdump, mdsz);

    *sz = sizeof(uint64_t)*2 + pgsz + mdsz;

    return packed;

}

bool grist_db_iterinit(grist_db* db) {
    return tchdbiterinit(db->hdb);
}

void* grist_db_iternext(grist_db* db, int* szp) {
    return tchdbiternext(db->hdb, szp);
}
