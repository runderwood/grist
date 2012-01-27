#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "geom.h"
#include "util.h"
#include "dict.h"
#include <math.h>
#include "db.h"

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

    /*grist_dict* d = grist_dict_new();

    int k = 0;
    size_t ksz = sizeof(int);
    int v = 0;
    size_t vsz = sizeof(int);

    int i = 0;
    int max = 100000;

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
    grist_dict_del(d);*/

    grist_linestr* l = grist_linestr_new();
    grist_linestr_addxyz(l, 0.0, 1.0, NAN);
    grist_linestr_addxyz(l, 1.0, 2.0, NAN);
    grist_linestr_addxyz(l, 2.0, 3.0, NAN);
    grist_linestr_addxyz(l, 3.0, 4.0, NAN);
    grist_linestr_addxyz(l, 0.0, 1.0, NAN);

    printf("linestr_sz: %d\n", grist_linestr_sz(l));

    if(grist_linestr_isring(l)) {
        printf("ring!\n");
    } else printf("not a ring.\n");

    size_t lssersz;
    char* lsser = grist_linestr_ser(l, &lssersz);

    printf("ser ls sz: %ld\n", lssersz);

    grist_linestr* l2 = grist_linestr_unser(lsser, lssersz);

    if(!l2) {
        printf("could not unserialize.\n");
        return EXIT_FAILURE;
    }

    printf("linestr2_sz: %d\n", grist_linestr_sz(l2));

    if(grist_linestr_isring(l2)) {
        printf("2 ring!\n");
    } else printf("2 not a ring.\n");

    //free(lsser);
    //lsser = NULL;

    grist_linestr_del(l);
    grist_linestr_del(l2);

    pid_t pid = getpid();
    char outfilename[256];
    sprintf(outfilename, "test-%d.grist", pid);
    grist_db* db = grist_db_new();
    printf("created db.\n");
    grist_db_open(db, outfilename, strlen(outfilename));
    printf("opened db: %s\n", outfilename);


    grist_point* p = grist_point_new3(0.0, 1.0, 3.0);

    // serialize point
    size_t serpsz;
    char* serp = grist_point_ser(p, &serpsz);

    grist_dict* d = grist_dict_new();
    grist_dict_set(d, "hello", 5, "world", 5);
    grist_dict_set(d, "goodbye", 7, "world", 5);

    // serialize dict
    size_t serdsz;
    char* serd = grist_dict_ser(d, &serdsz);

    printf("serialized point and dict: %d + %d = %d\n", serpsz, serdsz, serpsz+serdsz);

    grist_dict* d2 = grist_dict_unser(serd, serdsz);

    printf("unserialized dict with %d keys.\n", d2->cabsz - d2->empty);

    size_t vsz;
    char* val = grist_dict_get(d2, "hello", 5, &vsz);

    char* valstr = malloc(vsz+1);
    memcpy(valstr, val, vsz);
    valstr[vsz] = '\0';
    printf("got %s for key 'hello'.\n", valstr);
    
    grist_db_close(db);
    printf("closed db.\n");
    grist_db_del(db);
    printf("deld db.\n");
    db = NULL;

    return EXIT_SUCCESS;

}
