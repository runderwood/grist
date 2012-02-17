#ifndef GRIST_FTR_H
#define GRIST_FTR_H
#include <geos_c.h>

typedef struct grist_feature_s {
    GEOSGeometry* geom;
    json_object* attr;
} grist_feature;

grist_feature* grist_feature_new(void);
void grist_feature_del(grist_feature* f);

void* grist_feature_pack(const grist_feature* f, int* sz);
grist_feature* grist_feature_unpack(const void* v, int vsz);
char* grist_feature_tojson(grist_feature* f);
grist_feature* grist_feature_fromjson(const char* json);

#endif
