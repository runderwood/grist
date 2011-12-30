#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include "geom.h"

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
    }
    free(coordarr->arr);
    coordarr->tail = NULL;
    free(coordarr);
    return;
}

void grist_coordarr_push(grist_coordarr* coordarr, grist_coord* coord) {
    assert(coordarr && coordarr->sz >= 0);
    if(coordarr->sz >= coordarr->allocd) {
        coordarr->arr = realloc(coordarr->arr, sizeof(grist_coord)*coordarr->allocd*2);
        assert(coordarr->arr);
        coordarr->allocd = 2 * coordarr->allocd;
    }
    *(coordarr->arr+coordarr->sz) = coord;
    if(coordarr->sz == 0) {
        coordarr->head = coord;
    }
    coordarr->tail = coord;
    coordarr->sz++;
    return;
}

//grist_coordarr* grist_coord_pop(grist_coordarr coordarr);
//grist_coord* grist_coord_get(int idx);
