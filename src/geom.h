#ifndef GRIST_GEOM_H
#define GRIST_GEOM_H

#include <stdint.h>

#define GRIST_COORDARR_CHNK 8

#define GRIST_POINT 1
#define GRIST_LINESTRING 2
#define GRIST_POLYGON 4

typedef struct grist_coord_s {
    double x;
    double y;
    double z;
} grist_coord;

typedef struct grist_coordarr_s {
    uint32_t sz;
    int allocd;
    grist_coord** arr;
    grist_coord* head;
    grist_coord* tail;
} grist_coordarr;

typedef struct grist_point_s {
    grist_coord* coord;
} grist_point;

typedef struct grist_linestr_s {
    grist_coordarr* coords;
} grist_linestr;

typedef struct grist_polygon_s {
    grist_linestr* exterior;
    grist_linestr* interior;
} grist_polygon;

grist_coord* grist_coord_new(void);
grist_coord* grist_coord_new2(double x, double y);
grist_coord* grist_coord_new3(double x, double y, double z);
void grist_coord_del(grist_coord* coord);

grist_coordarr* grist_coordarr_new(void);
void grist_coordarr_del(grist_coordarr* coordarr);
void grist_coordarr_push(grist_coordarr* coordarr, grist_coord* coord);
grist_coord* grist_coordarr_pop(grist_coordarr* coordarr);

grist_point* grist_point_new(void);
grist_point* grist_point_new2(double x, double y);
grist_point* grist_point_new3(double x, double y, double z);
void grist_point_del(grist_point* p);

double grist_point_x(grist_point* p);
void grist_point_setx(grist_point* p, double x);
double grist_point_y(grist_point* p);
void grist_point_sety(grist_point* p, double y);
double grist_point_z(grist_point* p);
void grist_point_setz(grist_point* p, double z);

char* grist_point_ser(grist_point* p, size_t* sz);
grist_point* grist_point_unser(const char* buf, size_t bufsz);

grist_linestr* grist_linestr_new(void);
void grist_linestr_del(grist_linestr* l);
uint32_t grist_linestr_sz(grist_linestr* l); // todo: be sure we check for max size in all relevant linestr ops
uint32_t grist_linestr_addxyz(grist_linestr* l, double x, double y, double z);
uint32_t grist_linestr_addcoord(grist_linestr* l, grist_coord* c);
uint32_t grist_linestr_addpoint(grist_linestr* l, grist_point* p);
int grist_linestr_isring(grist_linestr* l);

char* grist_linestr_ser(const grist_linestr* l, size_t* sz);
grist_linestr* grist_linestr_unser(const char* buf, size_t bufsz);

grist_polygon* grist_polygon_new(void);
void grist_polygon_del(grist_polygon* pg);
double grist_polygon_area(grist_polygon* pg);

#endif
