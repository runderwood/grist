#include "feature.h"

grist_feature* grist_feature_new(void) {
    grist_feature* f = malloc(sizeof(grist_feature));
    assert(f);
    return f;
}

void grist_feature_del(grist_feature* f) {
    if(f->geom) GEOSGeometry_destroy(f->geom);
    if(f->attr) tcmapdel(f->attr);
    free(f);
    return;
}
