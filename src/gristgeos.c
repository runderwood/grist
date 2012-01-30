#include <geos_c.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {

    initGEOS((GEOSMessageHandler)printf, (GEOSMessageHandler)printf);

    GEOSCoordSequence* s = GEOSCoordSeq_create(1, 3);
    GEOSCoordSeq_setX(s, 0, -140.0);
    GEOSCoordSeq_setY(s, 0, 44.0);
    GEOSCoordSeq_setZ(s, 0, 12.0);
    GEOSGeometry* p = GEOSGeom_createPoint(s);

    GEOSWKTWriter* w = GEOSWKTWriter_create();

    char* wkt = GEOSWKTWriter_write(w, p);

    printf("point wkt: %s\n", wkt);

    free(wkt);
    wkt = NULL;

    GEOSCoordSequence* s2 = GEOSCoordSeq_create(2, 3);
    GEOSCoordSeq_setX(s2, 0, -140.0);
    GEOSCoordSeq_setY(s2, 0, 34.0);
    GEOSCoordSeq_setX(s2, 1, -136.0);
    GEOSCoordSeq_setY(s2, 1, 46.0);
    GEOSGeometry* l = GEOSGeom_createLineString(s2);

    wkt = GEOSWKTWriter_write(w, l);

    printf("line wkt: %s\n", wkt);

    free(wkt);

    double d;
    GEOSDistance(p, l, &d);

    printf("distance: %.02f\n", d);

    char r = GEOSIntersects(p, l);

    printf("intersects?: %d\n", (int)r);

    GEOSGeom_destroy(p);
    GEOSGeom_destroy(l);

    finishGEOS();
    return EXIT_SUCCESS;

}
