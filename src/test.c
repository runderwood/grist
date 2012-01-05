#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "geom.h"

void printcoord(grist_coord* coord) {
    printf("coord(%f,%f,%f)\n", coord->x, coord->y, coord->z);
    return;
}

void printpoint(grist_point* point) {
    printf("point(%f,%f,%F)\n", grist_point_x(point), grist_point_y(point), grist_point_z(point));
    return;
}

int main(int argc, char** argv) {
/*    grist_coord* coord = grist_coord_new();
    coord->x = 0.0;
    coord->y = 0.0;
    coord->x = -71.0;
    printcoord(coord);
    grist_coordarr* carr = grist_coordarr_new();
    printf("carr->sz = %d\n", carr->sz);
    grist_coordarr_push(carr, coord);
    printf("carr->sz = %d\n", carr->sz);
*/

/*int i;
    for(i=0; i<10; i++) {
        grist_coordarr_push(carr, grist_coord_new2(2.0,3.0));
        printf("carr->sz = %d (allocd = %d)\n", carr->sz, carr->allocd);
    }
    grist_coord* popped = NULL;
    while((popped = grist_coordarr_pop(carr))) {
        printf("popped: %f,%f,%f\n", popped->x, popped->y, popped->z);
        printf("carr->sz = %d (allocd = %d)\n", carr->sz, carr->allocd);
        grist_coord_del(popped);
    }
    grist_coordarr_del(carr);
*/

    grist_point* point = grist_point_new3(1.0, 2.0, 3.0);
    //printpoint(point);
    //grist_point_setx(point, 2.0);
    //printpoint(point);
    //size_t sersz = 0;
    //char* serd = grist_point_ser(point, &sersz);
    //grist_point* point2 = grist_point_unser(serd, sersz);
    //printpoint(point2);

    pid_t pid = getpid();
    char outfilename[256];
    sprintf(outfilename, "test-%d.bin", pid);
    FILE* outfile = fopen(outfilename, "w+");
    
    size_t sersz = 0;
    char* serd = grist_point_ser(point, &sersz);

    int byteswritten = fwrite(serd, 1, sersz, outfile);
    printf("Wrote %d bytes to %s.\n", byteswritten, outfilename);

    rewind(outfile);
    char* raw = malloc(sizeof(char)*sersz);
    size_t bytesread = fread(raw, 1, sersz, outfile);
    printf("Read %d bytes from %s.\n", (int)bytesread, outfilename);

    fclose(outfile);

    grist_point* point2 = grist_point_unser(raw, bytesread);
    printpoint(point2);

    grist_point_del(point);
    grist_point_del(point2);

    point = NULL;

}
