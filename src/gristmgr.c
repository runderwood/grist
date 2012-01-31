#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <tchdb.h>
#include <geos_c.h>
#include "util.h"

#define GRISTMGR_DEBUG 1

#define GRISTMGR_ERR 1
#define GRISTMGR_WRN 2
#define GRISTMGR_INF 4

#define GRISTMGR_INIT 1
#define GRISTMGR_GETX 2
#define GRISTMGR_SETX 3
#define GRISTMGR_PUTX 4
#define GRISTMGR_DELX 5
#define GRISTMGR_UNSE 6
#define GRISTMGR_STAT 7
#define GRISTMGR_LIST 8

static int verbose_flag = 0;
static int action_flag = 0;
static char* rec_key = NULL;
static char* rec_geom = NULL;

static int gristmgr_pid = 0;

/* private function prototypes */
void report(int mtype, char* msg, ...);
void usage();
int parseopts(int argc, char** argv);
static int docreate(char* fname);
static int dostatus(char* fname);
static int doput(char* fname, char* key, char* wktgeom);
static int doget(char* fname, char* key);

static char* gristmgr_pack_rec(char* wkt, TCMAP* map, size_t* sz);


int main(int argc, char** argv) {
    
    gristmgr_pid = getpid();

    initGEOS((GEOSMessageHandler)printf, (GEOSMessageHandler)printf);

    int optind = parseopts(argc, argv);
    if(optind >= argc) {
        usage();
        exit(EXIT_FAILURE);
    }

    int fnamesz = strlen(argv[argc-1])+1;
    char* fname = malloc(fnamesz);
    strcpy(fname, argv[argc-1]);
    fname[fnamesz-1] = '\0';

    switch(action_flag) {
        case GRISTMGR_INIT:
            docreate(fname);
            break;
        case GRISTMGR_STAT:
            dostatus(fname);
            break;
        case GRISTMGR_PUTX:
            doput(fname, rec_key, rec_geom);
            break;
        case GRISTMGR_GETX:
            doget(fname, rec_key);
            break;
        default:
            report(GRISTMGR_ERR, "invalid action");
            exit(EXIT_FAILURE);
    }

    finishGEOS();
    return EXIT_SUCCESS;   
}

void report(int mtype, char* msg, ...) {
    if((mtype & GRISTMGR_ERR) && (GRISTMGR_DEBUG || verbose_flag)) {
        va_list fmtargs;
        char buf[80];
        va_start(fmtargs, msg);
        vsnprintf(buf, 79, msg, fmtargs);
        va_end(fmtargs);
        fprintf(stderr, "%s\n", buf);
    }
    return;
}

void usage() {
    fprintf(stderr, "gristmgr v0.01\n\nusage: gristmgr <options ...> dbfilename\n\n");
    fprintf(stderr, "\t--verbose\t\t\tverbose mode\n");
    fprintf(stderr, "\t--brief\t\t\t\tbrief mode\n");
    fprintf(stderr, "\t--init\t\t\t\tinitialize database\n");
    fprintf(stderr, "\t--stat\t\t\t\tdb status\n");
    fprintf(stderr, "\t--list\t\t\t\tlist features\n");
    fprintf(stderr, "\t--geom\t\t\t\tWKT geometry\n");
    fprintf(stderr, "\t--key\t\t\t\tkey of record to manipulate\n");
    fprintf(stderr, "\n");
    return;
}

int parseopts(int argc, char** argv) {
    
    int c;
    
    static struct option lopts[] = {
        {"verbose", no_argument, &verbose_flag, 1},
        {"brief", no_argument, &verbose_flag, 0},
        {"init", no_argument, &action_flag, GRISTMGR_INIT},
        {"stat", no_argument, &action_flag, GRISTMGR_STAT},
        {"put", no_argument, &action_flag, GRISTMGR_PUTX},
        {"get", no_argument, &action_flag, GRISTMGR_GETX},
        {"key", required_argument, 0, 'k'},
        {"geom", required_argument, 0, 'g'},
        {NULL, 0, NULL, 0}
    };

    while(1) {
        int opt_idx = 0;

        c = getopt_long(argc, argv, "", lopts, &opt_idx);

        if(c == -1) break;

        int ksz;
        int gsz;

        switch(c) {
            case 0:
                break;
            case 'k':
                ksz = strlen(optarg);
                rec_key = malloc(ksz+1);
                strncpy(rec_key, optarg, ksz);
                rec_key[ksz] = '\0';
                break;
            case 'g':
                gsz = strlen(optarg);
                rec_geom = malloc(gsz+1);
                strncpy(rec_geom, optarg, gsz);
                rec_geom[gsz] = '\0';
                break;
            default:
                usage();
                abort();
                break;
        }

    }

    return optind;
}

static int docreate(char* fname) {
    TCHDB* hdb = tchdbnew();
    if(!tchdbtune(hdb, 32, -1, -1, HDBTLARGE | HDBTBZIP)) {
        tchdbdel(hdb);
        report(GRISTMGR_ERR, "could not tune db: %s", fname);
        exit(EXIT_FAILURE);
    }
    if(!tchdbopen(hdb, fname, HDBOWRITER | HDBOCREAT)) {
        tchdbdel(hdb);
        report(GRISTMGR_ERR, "could not open db: %s", fname);
        exit(EXIT_FAILURE);
    }
    tchdbclose(hdb);
    tchdbdel(hdb);
    hdb = NULL;
    return 0;
}

static int dostatus(char* fname) {
    TCHDB* hdb = tchdbnew();
    if(!tchdbopen(hdb, fname, HDBOREADER)) {
        tchdbdel(hdb);
        report(GRISTMGR_ERR, "could not open db: %s", fname);
        exit(EXIT_FAILURE);
    }
    const char* dbpath = tchdbpath(hdb);
    if(!dbpath) dbpath = "(unknown)";
    printf("path: %s\n", dbpath);
    printf("feature count: %llu\n", (long long unsigned)tchdbrnum(hdb));
    printf("file size: %llu\n", (long long unsigned)tchdbfsiz(hdb));
    tchdbclose(hdb);
    tchdbdel(hdb);
    hdb = NULL;
    return 0;
}

static int doput(char* fname, char* k, char* wkt) {

    TCHDB* hdb = tchdbnew();
    if(!tchdbopen(hdb, fname, HDBOWRITER)) {
        tchdbdel(hdb);
        report(GRISTMGR_ERR, "could not open db: %s", fname);
        exit(EXIT_FAILURE);
    }

    TCMAP* map = tcmapnew();
    tcmapput(map, "hello", 5, "world", 5);

    size_t packedsz;
    char* packed = gristmgr_pack_rec(wkt, map, &packedsz);
    if(!packed) {
        tchdbclose(hdb);
        tchdbdel(hdb);
        report(GRISTMGR_ERR, "could not pack record: %s", k);
        exit(EXIT_FAILURE);
    }

    if(!tchdbput(hdb, k, strlen(k), packed, packedsz)) {
        report(GRISTMGR_ERR, "could not update %s: %s", k, tchdberrmsg(tchdbecode(hdb)));
        tchdbclose(hdb);
        tchdbdel(hdb);
        exit(EXIT_FAILURE);
    }

    tcmapdel(map);
    free(packed);

    tchdbclose(hdb);
    tchdbdel(hdb);

    return 0;
}

static int doget(char* fname, char* k) {
    printf("doget\n");

    TCHDB* hdb = tchdbnew();
    if(!tchdbopen(hdb, fname, HDBOWRITER)) {
        tchdbdel(hdb);
        report(GRISTMGR_ERR, "could not open db: %s", fname);
        exit(EXIT_FAILURE);
    }

    int vsz;
    void* v = tchdbget(hdb, k, strlen(k), &vsz);
    if(!v) {
        report(GRISTMGR_ERR, "could not get rec %s: %s", k, tchdberrmsg(tchdbecode(hdb)));
        tchdbclose(hdb);
        tchdbdel(hdb);
        exit(EXIT_FAILURE);
    }

    uint64_t pgsz;
    memcpy(&pgsz, v, sizeof(uint64_t));
    uint64_t mdsz;
    memcpy(&mdsz, v+sizeof(uint64_t), sizeof(uint64_t));

    pgsz = ntohll(pgsz);
    mdsz = ntohll(mdsz);

    GEOSWKBReader* r = GEOSWKBReader_create();

    GEOSGeometry* g = GEOSWKBReader_read(r, v+(2*sizeof(uint64_t)), pgsz);
    if(!g) {
        report(GRISTMGR_ERR, "could not unpack geometry %s", k);
        tchdbclose(hdb);
        tchdbdel(hdb);
        abort();
        exit(EXIT_FAILURE);
    }

    TCMAP* map = tcmapload(v+(2*sizeof(uint64_t))+pgsz, mdsz);
    if(!map) {
        report(GRISTMGR_ERR, "could not unpack attributes");
        tchdbclose(hdb);
        tchdbdel(hdb);
        exit(EXIT_FAILURE);
    }

    GEOSWKTWriter* w = GEOSWKTWriter_create();
    char* wkt = GEOSWKTWriter_write(w, g);
    if(!wkt) {
        abort();
    }

    printf("%s @ %s\n", k, wkt);

    tcmapiterinit(map);
    const char* mk;
    int mksz;
    const char* mv;
    int mvsz;
    while((mk = tcmapiternext(map, &mksz))) {
        mv = tcmapget(map, mk, mksz, &mvsz);
        printf("%s:\n\t%s\n", mk, mv);
    }

    return 0;

}


/* util */
static char* gristmgr_pack_rec(char* wkt, TCMAP* map, size_t* sz) {
    char* packed = NULL;

    GEOSWKTReader* r = GEOSWKTReader_create();
    GEOSGeometry* g = GEOSWKTReader_read(r, wkt);
    if(!g) {
        report(GRISTMGR_ERR, "could not parse wkt geom: %s", wkt);
        return packed;
    }

    GEOSWKBWriter* w = GEOSWKBWriter_create();
    GEOSWKBWriter_setByteOrder(w, GEOS_WKB_XDR);

    size_t pgsz;
    unsigned char* wkb = GEOSWKBWriter_write(w, g, &pgsz);
    if(!wkb || !pgsz) {
        report(GRISTMGR_ERR, "could not write wkb");
        return packed;
    }

    int mdsz;
    char* mapdump = tcmapdump(map, &mdsz);
    if(!mapdump || !mdsz) {
        report(GRISTMGR_ERR, "could not dump attributes");
        return packed;
    }

    *sz = sizeof(uint64_t)*2 + pgsz + mdsz;

    packed = malloc(*sz);
    if(!packed) {
        report(GRISTMGR_ERR, "could not serialize: out of memory");
        return packed;
    }

    uint64_t pgsz_ = htonll((uint64_t)pgsz);
    uint64_t mdsz_ = htonll((uint64_t)mdsz);

    size_t offset = 0;
    memcpy(packed, &pgsz_, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    memcpy(packed+offset, &mdsz_, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    memcpy(packed+offset, wkb, pgsz);
    offset += pgsz;
    memcpy(packed+offset, mapdump, mdsz);

    return packed;
}
