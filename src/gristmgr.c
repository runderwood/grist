#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <tchdb.h>
#include <geos_c.h>
#include <json/json.h>
#include "util.h"
#include "db.h"

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
static char* rec_json = NULL;

static int gristmgr_pid = 0;

/* private function prototypes */
void report(int mtype, char* msg, ...);
void usage();
int parseopts(int argc, char** argv);
static int docreate(char* fname);
static int dostatus(char* fname);
static int doput(char* fname, char* key, char* wktgeom, char* attrs);
static int doget(char* fname, char* key);
static int dolist(char* fname);

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
        case GRISTMGR_LIST:
            dolist(fname);
            break;
        case GRISTMGR_PUTX:
            doput(fname, rec_key, rec_geom, rec_json);
            break;
        case GRISTMGR_GETX:
            doget(fname, rec_key);
            break;
        default:
            report(GRISTMGR_ERR, "invalid action");
            exit(EXIT_FAILURE);
    }

    free(fname);

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
        {"list", no_argument, &action_flag, GRISTMGR_LIST},
        {"put", no_argument, &action_flag, GRISTMGR_PUTX},
        {"get", no_argument, &action_flag, GRISTMGR_GETX},
        {"key", required_argument, 0, 'k'},
        {"geom", required_argument, 0, 'g'},
        {"json", required_argument, 0, 'j'},
        {NULL, 0, NULL, 0}
    };

    while(1) {
        int opt_idx = 0;

        c = getopt_long(argc, argv, "", lopts, &opt_idx);

        if(c == -1) break;

        int ksz;
        int gsz;
        int jsz;

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
            case 'j':
                jsz = strlen(optarg);
                rec_json = malloc(jsz+1);
                strncpy(rec_json, optarg, jsz);
                rec_json[jsz] = '\0';
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

static int doput(char* fname, char* k, char* wkt, char* attrs) {

    if(!k || !wkt || !attrs) {
        report(GRISTMGR_ERR, "missing arg");
        exit(EXIT_FAILURE);
    }

    grist_db* db = grist_db_new();

    if(!grist_db_open(db, fname, HDBOWRITER)) {
        grist_db_del(db);
        report(GRISTMGR_ERR, "could not open db: %s", fname);
        exit(EXIT_FAILURE);
    }

    GEOSWKTReader* r = GEOSWKTReader_create();
    GEOSGeometry* g = GEOSWKTReader_read(r, wkt);

    if(!g) {
        report(GRISTMGR_ERR, "invalid geometry");
        exit(EXIT_FAILURE);
    }

    grist_feature* f = grist_feature_new();
    f->geom = g;
    f->data = json_tokener_parse(attrs);

    if(!grist_db_put(db, k, strlen(k), f)) {
        report(GRISTMGR_ERR, "could not update %s: %s", k, tchdberrmsg(tchdbecode(db->hdb)));
        grist_db_close(db);
        grist_db_del(db);
        exit(EXIT_FAILURE);
    }

    grist_db_close(db);
    grist_db_del(db);

    return 0;
}

static int doget(char* fname, char* k) {

    if(!k) {
        report(GRISTMGR_ERR, "missing key");
        exit(EXIT_FAILURE);
    }
    
    grist_db* db = grist_db_new();
    if(!grist_db_open(db, fname, HDBOREADER)) {
        abort();
    }

    grist_feature* f = grist_db_get(db, k, strlen(k));
    if(!f) {
        abort();
    }

    GEOSWKTWriter* w = GEOSWKTWriter_create();
    char* wkt = GEOSWKTWriter_write(w, f->geom);
    if(!wkt) {
        abort();
    }

    printf("%s @ %s\n", k, wkt);

    /*tcmapiterinit(f->attr);
    const char* mk;
    int mksz;
    const char* mv;
    int mvsz;
    while((mk = tcmapiternext(f->attr, &mksz))) {
        mv = tcmapget(f->attr, mk, mksz, &mvsz);
        printf("%s:\n\t%s\n", mk, mv);
    }*/

    return 0;

}

int dolist(char* fname) {
    
    grist_db* db = grist_db_new();
    if(!grist_db_open(db, fname, HDBOREADER)) {
        abort();
    }

    grist_db_iterinit(db);

    int ksz;
    char* k;
    int i = 0;
    grist_feature* f;
    char* wkt;
    GEOSWKTWriter* w = GEOSWKTWriter_create();
    while((k = grist_db_iternext(db, &ksz))) {
        f = grist_db_get(db, k, ksz);
        wkt = GEOSWKTWriter_write(w, f->geom);
        printf("%d:\t%s, %s, %s\n", ++i, k, wkt, f->data ? json_object_to_json_string(f->data): NULL);
        grist_feature_del(f);
        f = NULL;
        free(wkt);
        wkt = NULL;
        free(k);
        k = NULL;
    }

    GEOSWKTWriter_destroy(w);

    grist_db_close(db);
    grist_db_del(db);

    return 0;

}
