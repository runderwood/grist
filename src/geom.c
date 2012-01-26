#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <netinet/in.h>
#include <assert.h>
#include "geom.h"
#include "util.h"
#include <stdio.h>

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

int grist_coord_eq(grist_coord* c0, grist_coord* c1) {
    if((c0->x == c1->x || (isnan(c0->x) && isnan(c1->x))) &&
        (c0->y == c1->y || (isnan(c0->y) && isnan(c1->y))) &&
        (c0->z == c1->z || (isnan(c0->z) && isnan(c1->z)))) {
        return 1;
    }
    return 0;
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

char* grist_point_ser(grist_point* p, size_t* sz) {
    size_t newsz = sizeof(grist_coord);
    char* serialized = malloc(newsz);
    assert(serialized);
    uint64_t x;
    uint64_t y;
    uint64_t z;
    x = htonll(dtoll(p->coord->x));
    y = htonll(dtoll(p->coord->y));
    z = htonll(dtoll(p->coord->z));
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
    grist_point* point = grist_point_new3(
        lltod(ntohll(x)), lltod(ntohll(y)), lltod(ntohll(z))
    );
    return point;
}

grist_linestr* grist_linestr_new(void) {
    grist_linestr* l = malloc(sizeof(grist_linestr));
    assert(l);
    l->coords = grist_coordarr_new();
    return l;
}

void grist_linestr_del(grist_linestr* l) {
    assert(l && l->coords);
    grist_coordarr_del(l->coords);
    l->coords = NULL;
    free(l);
    l = NULL;
    return;
}

uint32_t grist_linestr_sz(grist_linestr* l) {
    return l->coords->sz;
}

int grist_linestr_isring(grist_linestr* l) {
    assert(l && l->coords);
    if(l->coords->sz > 3) {
        grist_coord* c0 = l->coords->arr[0];
        grist_coord* cN = l->coords->arr[l->coords->sz-1];
        if(grist_coord_eq(c0, cN)) {
            return 1;
        }
    }
    return 0;
}

uint32_t grist_linestr_pushcoord(grist_linestr* l, grist_coord* c) {
    assert(l->coords->sz < UINT32_MAX);
    grist_coordarr_push(l->coords, c);
    return l->coords->sz;
}

uint32_t grist_linestr_addxyz(grist_linestr* l, double x, double y , double z) {
    grist_coord* c = grist_coord_new3(x, y, z);
    return grist_linestr_pushcoord(l, c);
}

uint32_t grist_linestr_addpoint(grist_linestr* l, grist_point* p) {
    return grist_linestr_addxyz(l, p->coord->x, p->coord->y, p->coord->z);
}

char* grist_linestr_ser(const grist_linestr* l, size_t* sz) {
    size_t newsz = 4+sizeof(grist_coord)*l->coords->sz; // sz + coords
    char* serialized = malloc(newsz);
    assert(serialized);
    uint64_t x;
    uint64_t y;
    uint64_t z;
    uint32_t i;
    grist_coord* c;
    uint32_t lsz = htonl(l->coords->sz);
    memcpy(serialized, &lsz, sizeof(uint32_t));
    size_t offset = sizeof(uint32_t);
    for(i=0; i<l->coords->sz; i++) {
        c = l->coords->arr[i];
        x = htonll(dtoll(c->x));
        y = htonll(dtoll(c->y));
        z = htonll(dtoll(c->z));
        memcpy((serialized+offset), &x, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy((serialized+offset), &y, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy((serialized+offset), &z, sizeof(uint64_t));
        offset += sizeof(uint64_t);
    }
    *sz = newsz;
    return serialized;
}

grist_linestr* grist_linestr_unser(const char* buf, size_t bufsz) {
    uint32_t lsz;
    memcpy(&lsz, buf, sizeof(uint32_t));
    lsz = ntohl(lsz);
    size_t sersz = sizeof(uint32_t)+sizeof(grist_coord)*lsz;
    if(bufsz != sersz) {
        return NULL;
    }
    size_t offset = sizeof(uint32_t);
    grist_linestr* l = grist_linestr_new();
    uint32_t i;
    for(i=0; i<lsz; i++) {
        uint64_t x;
        uint64_t y;
        uint64_t z;
        memcpy(&x, (buf+offset), sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy(&y, (buf+offset), sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy(&z, (buf+offset), sizeof(uint64_t));
        offset += sizeof(uint64_t);
        grist_linestr_addxyz(l, 
            lltod(ntohll(x)), lltod(ntohll(y)), lltod(ntohll(z))
        );
    }
    return l;
}
