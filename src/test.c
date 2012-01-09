#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "geom.h"
#include "util.h"
#include "dict.h"

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

    //grist_point* point = grist_point_new3(1.0, 2.0, 3.0);
    //printpoint(point);
    //grist_point_setx(point, 2.0);
    //printpoint(point);
    //size_t sersz = 0;
    //char* serd = grist_point_ser(point, &sersz);
    //grist_point* point2 = grist_point_unser(serd, sersz);
    //printpoint(point2);
    
    /*printpoint(point);

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

    free(raw);
    free(serd);

    point = NULL;

    char s1[4] = "reed";
    char s2[4] = "REED";
    printf("hashed %.*s: %llu\n", 4, s1, (long long unsigned int)djb_hash((uint8_t*)s1, 4));
    printf("hashed %.*s: %llu\n", 4, s2, (long long unsigned int)djb_hash((uint8_t*)s2, 4));

    */

    grist_dict* d = grist_dict_new();

    int k = 0;
    size_t ksz = sizeof(int);
    int v = 0;
    size_t vsz = sizeof(int);

    int i = 0;
    int max = 10000;

    for(i=0; i<max; i++) {
        k = i;
        v = i;
        grist_dict_set(d, &k, ksz, &v, vsz);
    }

    size_t v2sz;
    k = i / 2;
    void* v2 = grist_dict_get(d, &k, ksz, &v2sz);
    printf("got %04X: %04X\n", k, *((int*)v2));
    free(v2);
    v2 = NULL;

    free(v2);
    printf("d->sz: %zu\n", d->cabsz);
    grist_dict_del(d);

}
