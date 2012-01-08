#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "geom.h"
#include "util.h"

grist_coord* grist_coord_new(void) {
    grist_coord* coord = malloc(sizeof(grist_coord));
    assert(coord);
    coord->x = NAN;
    coord->y = NAN;
    coord->z = NAN;
    return coord;
}

grist_coord* grist_coord_new2(double x, double y) {
    grist_coord* coord = grist_coord_new();
    coord->x = x;
    coord->y = y;
    return coord;
}

grist_coord* grist_coord_new3(double x, double y, double z) {
    grist_coord* coord = grist_coord_new();
    coord->x = x;
    coord->y = y;
    coord->z = z;
    return coord;
}

void grist_coord_del(grist_coord* coord) {
    free(coord);
    return;
}

grist_coordarr* grist_coordarr_new(void) {
    grist_coordarr* coordarr = malloc(sizeof(grist_coordarr));
    assert(coordarr);
    coordarr->sz = 0;
    coordarr->allocd = 0;
    coordarr->arr = malloc(sizeof(grist_coord)*GRIST_COORDARR_CHNK);
    assert(coordarr->arr);
    coordarr->allocd = GRIST_COORDARR_CHNK;
    coordarr->head = NULL;
    coordarr->tail = NULL;
    return coordarr;
}

void grist_coordarr_del(grist_coordarr* coordarr) {
    int i;
    grist_coord* coord;
    coordarr->head = NULL;
    for(i=0; i<coordarr->sz; i++) {
        coord = *(coordarr->arr+i);
        free(coord);
        coord = NULL;
    }
    free(coordarr->arr);
    coordarr->tail = NULL;
    free(coordarr);
    coordarr = NULL;
    return;
}

void grist_coordarr_push(grist_coordarr* coordarr, grist_coord* coord) {
    assert(coordarr && coordarr->sz >= 0);
    if(coordarr->sz >= coordarr->allocd) {
        coordarr->arr = realloc(coordarr->arr, sizeof(grist_coord*)*coordarr->allocd*2);
        assert(coordarr->arr);
        coordarr->allocd = 2 * coordarr->allocd;
    }
    coordarr->arr[coordarr->sz] = coord;
    if(coordarr->sz == 0) {
        coordarr->head = coord;
    }
    coordarr->tail = coord;
    coordarr->sz++;
    return;
}

grist_coord* grist_coordarr_pop(grist_coordarr* coordarr) {
    assert(coordarr);
    grist_coord* popped = NULL;
    if(coordarr->sz > 0) {
        popped = coordarr->tail;
        if(coordarr->sz > 1) {
            coordarr->tail = coordarr->arr[coordarr->sz-2];
        } else {
            coordarr->tail = NULL;
            coordarr->head = NULL;
        }
        coordarr->sz--;
        if(coordarr->sz < (coordarr->allocd / 2) && coordarr->allocd >= (2*GRIST_COORDARR_CHNK)) {
            coordarr->arr = realloc(coordarr->arr, sizeof(grist_coord*)*coordarr->allocd/2);
            assert(coordarr->arr);
            coordarr->allocd = coordarr->allocd/2;
        }
    }
    return popped;
}

grist_point* grist_point_new(void) {
    grist_point* point = malloc(sizeof(grist_point));
    assert(point);
    point->geom_type = GRIST_POINT;
    point->coord = grist_coord_new();
    return point;
}

grist_point* grist_point_new2(double x, double y) {
    grist_point* point = grist_point_new();
    point->coord->x = x;
    point->coord->y = y;
    return point;
}

grist_point* grist_point_new3(double x, double y, double z) {
    grist_point* point = grist_point_new2(x, y);
    point->coord->z = z;
    return point;
}

void grist_point_del(grist_point* point) {
    assert(point);
    free(point->coord);
    point->coord = NULL;
    free(point);
    point = NULL;
    return;
}

double grist_point_x(grist_point* point) {
    assert(point);
    return point->coord->x;
}

void grist_point_setx(grist_point* point, double x) {
    assert(point);
    point->coord->x = x;
    return;
}

double grist_point_y(grist_point* point) {
    assert(point);
    return point->coord->y;
}

void grist_point_sety(grist_point* point, double y) {
    assert(point);
    point->coord->y = y;
    return;
}

double grist_point_z(grist_point* point) {
    assert(point);
    return point->coord->z;
}

void grist_point_setz(grist_point* point, double z) {
    assert(point);
    point->coord->z = z;
    return;
}

char* grist_point_ser(grist_point* point, size_t* sz) {
    size_t newsz = sizeof(grist_coord);
    char* serialized = malloc(newsz);
    assert(serialized);
    uint64_t x;
    uint64_t y;
    uint64_t z;
    x = htonll(dtoll(point->coord->x));
    y = htonll(dtoll(point->coord->y));
    z = htonll(dtoll(point->coord->z));
    memcpy(serialized, &x, sizeof(uint64_t));
    size_t offset = sizeof(uint64_t);
    memcpy((serialized+offset), &y, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    memcpy((serialized+offset), &z, sizeof(uint64_t));
    *sz = newsz;
    return serialized;
}

grist_point* grist_point_unser(const char* buf, size_t bufsz) {
    size_t sersz = sizeof(grist_coord);
    if(bufsz != sersz) {
        return NULL;
    }
    uint64_t x;
    uint64_t y;
    uint64_t z;
    memcpy(&x, buf, sizeof(uint64_t));
    memcpy(&y, (buf+sizeof(uint64_t)), sizeof(uint64_t));
    memcpy(&z, (buf+(2*sizeof(uint64_t))), sizeof(uint64_t));
    grist_point* point = grist_point_new3(lltod(ntohll(x)), lltod(ntohll(y)), lltod(ntohll(z)));
    return point;
}
