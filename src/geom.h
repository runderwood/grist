#ifndef GRIST_GEOM_H
#define GRIST_GEOM_H

#define GRIST_COORDARR_CHNK 8

#define GRIST_POINT 1
#define GRIST_LINESTRING 2

typedef struct grist_coord_s {
    double x;
    double y;
    double z;
} grist_coord;

grist_coord* grist_coord_new(void);
grist_coord* grist_coord_new2(double x, double y);
grist_coord* grist_coord_new3(double x, double y, double z);
void grist_coord_del(grist_coord* coord);

typedef struct grist_coordarr_s {
    int sz;
    int allocd;
    grist_coord** arr;
    grist_coord* head;
    grist_coord* tail;
} grist_coordarr;

grist_coordarr* grist_coordarr_new(void);
void grist_coordarr_del(grist_coordarr* coordarr);
void grist_coordarr_push(grist_coordarr* coordarr, grist_coord* coord);
grist_coordarr* grist_coordarr_pop(grist_coordarr coordarr);
grist_coord* grist_coordarr_get(int idx);

#endif
