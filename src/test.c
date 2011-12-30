#include <stdio.h>
#include "geom.h"

void printcoord(grist_coord* coord) {
    printf("coord(%f,%f,%f)\n", coord->x, coord->y, coord->z);
    return;
}

int main(int argc, char** argv) {
    grist_coord* coord = grist_coord_new();
    coord->x = 0.0;
    coord->y = 0.0;
    coord->x = -71.0;
    printcoord(coord);
    grist_coordarr* carr = grist_coordarr_new();
    printf("carr->sz = %d\n", carr->sz);
    grist_coordarr_push(carr, coord);
    printf("carr->sz = %d\n", carr->sz);
    int i;
    for(i=0; i<100; i++) {
        grist_coordarr_push(carr, grist_coord_new2(2.0,3.0));
        printf("carr->sz = %d\n", carr->sz);
    }
    grist_coordarr_del(carr);
}
