#ifndef GRIST_FTR_H
#define GRIST_FTR_H
#include <geos_c.h>
#include <tchdb.h>

typedef struct grist_feature_s {
    GEOSGeometry* geom;
    TCMAP* attr;
} grist_feature;

grist_feature* grist_feature_new(void);
void grist_feature_del(grist_feature* f);

#endif
