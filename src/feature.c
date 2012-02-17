#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <json/json.h>
#include "feature.h"
#include "util.h"

#include <stdio.h>

grist_feature* grist_feature_new(void) {
    grist_feature* f = malloc(sizeof(grist_feature));
    f->geom = NULL;
    f->attr = NULL;
    assert(f);
    return f;
}

void grist_feature_del(grist_feature* f) {
    if(f->geom) GEOSGeom_destroy(f->geom);
    //if(f->data) FREE JSON OBJ HERE?
    free(f);
    return;
}

void* grist_feature_pack(const grist_feature* f, int* sz) {
    assert(f && f->attr && f->geom);
    char* packed = NULL;

    GEOSWKBWriter* w = GEOSWKBWriter_create();
    GEOSWKBWriter_setByteOrder(w, GEOS_WKB_XDR);

    size_t pgsz;
    unsigned char* wkb = GEOSWKBWriter_write(w, f->geom, &pgsz);
    if(!wkb || !pgsz) {
        return packed;
    }

    const char* datadump = json_object_to_json_string(f->attr);
    if(!datadump) {
        return packed;
    }
    int pdsz = strlen(datadump);

    *sz = sizeof(uint64_t)*2 + pgsz + pdsz;
    packed = malloc(*sz);
    if(!packed) {
        return packed;
    }

    uint64_t pgsz_ = htonll((uint64_t)pgsz);
    uint64_t pdsz_ = htonll((uint64_t)pdsz);

    size_t offset = 0;
    memcpy(packed, &pgsz_, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    memcpy(packed+offset, &pdsz_, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    memcpy(packed+offset, wkb, pgsz);
    offset += pgsz;
    memcpy(packed+offset, datadump, pdsz);

    return packed;

}

grist_feature* grist_feature_unpack(const void* v, int vsz) {
    assert(v && vsz > 0);

    uint64_t pgsz;
    memcpy(&pgsz, v, sizeof(uint64_t));
    uint64_t pdsz;
    memcpy(&pdsz, v+sizeof(uint64_t), sizeof(uint64_t));

    pgsz = ntohll(pgsz);
    pdsz = ntohll(pdsz);

    GEOSWKBReader* r = GEOSWKBReader_create();

    GEOSGeometry* g = GEOSWKBReader_read(r, v+(2*sizeof(uint64_t)), pgsz);
    GEOSWKBReader_destroy(r);

    if(!g) {
        return NULL;
    }

    char* jsonstr = malloc(pdsz+1);
    if(!jsonstr) {
        GEOSGeom_destroy(g);
        return NULL;
    }

    json_object* fdata;
    if(pdsz>0) {
        memcpy(jsonstr, v+(2*sizeof(uint64_t))+pgsz, pdsz);
        jsonstr[pdsz] = '\0';
        fdata = json_tokener_parse(jsonstr);
        if(!fdata) {
            GEOSGeom_destroy(g);
            return NULL;
        }
    } else {
        fdata = json_object_new_object();
    }

    free(jsonstr);
    
    grist_feature* f = grist_feature_new();
    f->geom = g;
    f->attr = fdata;

    return f;
}

char* grist_feature_tojson(grist_feature* f) {
    json_object* fjsobj = json_object_new_object();
    GEOSWKTWriter* w = GEOSWKTWriter_create();
    char* wkt = GEOSWKTWriter_write(w, f->geom); // todo: geojson here.
    GEOSWKTWriter_destroy(w);
    json_object* gstr = json_object_new_string(wkt);
    free(wkt);
    json_object_object_add(fjsobj, "geom", gstr);
    json_object_object_add(fjsobj, "attr", f->attr);
    char* j = (char*)json_object_to_json_string(fjsobj);
    json_object_object_del(fjsobj, "geom");
    json_object_object_del(fjsobj, "attr");
    return j;
}

grist_feature* grist_feature_fromjson(const char* json) {
    grist_feature* f = NULL;
    json_object* jsonobj = json_tokener_parse(json);
    if(jsonobj) {
        f = grist_feature_new();
        json_object* geom = json_object_object_get(jsonobj, "geom");
        const char* wkt = json_object_get_string(geom);
        if(wkt) {
            GEOSWKTReader* r = GEOSWKTReader_create();
            GEOSGeometry* g = GEOSWKTReader_read(r, wkt);
            if(g) {
                f->geom = g;
                json_object* attr = json_object_get(json_object_object_get(jsonobj, "attr"));
                if(attr) {
                    f->attr = attr;
                } else {
                    GEOSGeom_destroy(g);
                    f->geom = NULL;
                }
            }
            GEOSWKTReader_destroy(r);
        }
        if(!f->attr || !f->geom) {
            f = NULL;
        }
    }
    json_object_put(jsonobj);
    return f;
}
