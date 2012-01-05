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

typedef struct grist_coordarr_s {
    int sz;
    int allocd;
    grist_coord** arr;
    grist_coord* head;
    grist_coord* tail;
} grist_coordarr;

typedef struct grist_point_s {
    int geom_type;    
    grist_coord* coord;
} grist_point;

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
void grist_point_del(grist_point* point);

double grist_point_x(grist_point* point);
void grist_point_setx(grist_point* point, double x);
double grist_point_y(grist_point* point);
void grist_point_sety(grist_point* point, double y);
double grist_point_z(grist_point* point);
void grist_point_setz(grist_point* point, double z);

char* grist_point_ser(grist_point* point, size_t* sz);
grist_point* grist_point_unser(const char* buf, size_t bufsz);

#endif
