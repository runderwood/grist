#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <geos_c.h>
#include <json/json.h>
#include <js/jsapi.h>
#include "db.h"
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
#define GRISTMGR_EVAL 9
#define GRISTMGR_MAPX 10
#define GRISTMGR_LOAD 11

static int verbose_flag = 1;
static int action_flag = 0;

/* private function prototypes */
static void report(int mtype, char* msg, ...);
static void usage();
static int parseopts(int argc, const char** argv);
static int docreate(int argc, const char** argv);
static int dostatus(int argc, const char** argv);
static int doput(int argc, const char** argv);
static int doget(int argc, const char** argv);
static int dolist(int argc, const char** argv);
static int doeval(int argc, const char** argv);
static int domap(int argc, const char** argv);
static int doload(int argc, const char** argv);

int main(int argc, const char** argv) {
    
    if(argc < 2 || !parseopts(argc, argv)) {
        usage();
        exit(EXIT_FAILURE);
    }
    
    initGEOS((GEOSMessageHandler)printf, (GEOSMessageHandler)printf);

    switch(action_flag) {
        case GRISTMGR_INIT:
            docreate(argc, argv);
            break;
        case GRISTMGR_STAT:
            dostatus(argc, argv);
            break;
        case GRISTMGR_LIST:
            dolist(argc, argv);
            break;
        case GRISTMGR_PUTX:
            doput(argc, argv);
            break;
        case GRISTMGR_GETX:
            doget(argc, argv);
            break;
        case GRISTMGR_EVAL:
            doeval(argc, argv);
            break;
        case GRISTMGR_MAPX:
            domap(argc, argv);
            break;
        case GRISTMGR_LOAD:
            doload(argc, argv);
            break;
        default:
            report(GRISTMGR_ERR, "invalid action");
            finishGEOS();
            exit(EXIT_FAILURE);
    }

    //free(fname);

    finishGEOS();
    return EXIT_SUCCESS;
}

void report(int mtype, char* msg, ...) {
    if((mtype & GRISTMGR_ERR) || (GRISTMGR_DEBUG || verbose_flag)) {
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
    fprintf(stderr, "gristmgr v0.01\nusage: gristmgr cmd [options...] path [params...]\n");
    fprintf(stderr, "\tinit path\n");
    fprintf(stderr, "\tstat path\n");
    fprintf(stderr, "\tlist [-pv] path\n");
    fprintf(stderr, "\tput path [key] wkt-geom json-object\n");
    fprintf(stderr, "\tget path [-wb|-js|-gj|-wt] key\n");
    fprintf(stderr, "\tdel path key\n");
    fprintf(stderr, "\teval path key script\n");
    fprintf(stderr, "\tmap path script\n");
    fprintf(stderr, "\tload path jsonfile\n");
    fprintf(stderr, "\tmkview path view [-lu|-js] script\n");
    fprintf(stderr, "\trmview path view\n");
    fprintf(stderr, "\tvget path view key\n");
    fprintf(stderr, "\tversion\n");
    fprintf(stderr, "\thelp\n");
    return;
}

static int parseopts(int argc, const char** argv) {
    if(!strcmp(argv[1], "init")) {
        action_flag = GRISTMGR_INIT;
    } else if(!strcmp(argv[1], "stat")) {
        action_flag = GRISTMGR_STAT;
    } else if(!strcmp(argv[1], "list")) {
        action_flag = GRISTMGR_LIST;
    } else if(!strcmp(argv[1], "put")) {
        action_flag = GRISTMGR_PUTX;
    } else if(!strcmp(argv[1], "get")) {
        action_flag = GRISTMGR_GETX;
    } else if(!strcmp(argv[1], "eval")) {
        action_flag = GRISTMGR_EVAL;
    } else if(!strcmp(argv[1], "map")) {
        action_flag = GRISTMGR_MAPX;
    } else if(!strcmp(argv[1], "load")) {
        action_flag = GRISTMGR_LOAD;
    }
    return action_flag;
}

static int docreate(int argc, const char** argv) {
    if(argc<3) goto opterr;
    const char* fname = argv[2];
    grist_db* db = grist_db_new();
    if(!grist_db_open(db, fname, BDBOWRITER|BDBOCREAT)) {
        grist_db_del(db);
        report(GRISTMGR_ERR, "could not open db: %s", fname);
        exit(EXIT_FAILURE);
    }
    grist_db_close(db);
    grist_db_del(db);
    db = NULL;
    return 0;

    opterr:
        finishGEOS();
        usage();
        return EXIT_FAILURE;
}

static int dostatus(int argc, const char** argv) {
    if(argc<3) goto opterr;
    const char* fname = argv[2];
    grist_db* db = grist_db_new();
    if(!grist_db_open(db, fname, BDBOREADER)) {
        grist_db_del(db);
        db = NULL;
        report(GRISTMGR_ERR, "could not open db: %s", fname);
        exit(EXIT_FAILURE);
    }
    printf("path: %s\n", fname);
    printf("feature count: %llu\n", (long long unsigned)grist_db_fcount(db));
    printf("file size: %llu\n", (long long unsigned)grist_db_filesz(db));
    grist_db_close(db);
    grist_db_del(db);
    db = NULL;
    return 0;

    opterr:
        usage();
        return EXIT_FAILURE;
}

static int doput(int argc, const char** argv) {
    if(argc<5) goto opterr;

    const char* fname = argv[2];
    char k[65];
    const char* wkt = argv[4];
    const char* attrs = argv[5];

    if(argc < 6) {
        fname = argv[2];
        wkt = argv[3];
        attrs = argv[4];
        k[0] = '\0';
    } else {
        fname = argv[2];
        strncpy(k, argv[3], 64);
        k[64] = '\0';
        wkt = argv[4];
        attrs = argv[5];
    }

    grist_db* db = grist_db_new();

    if(!grist_db_open(db, fname, BDBOWRITER)) {
        grist_db_del(db);
        report(GRISTMGR_ERR, "could not open db: %s", fname);
        exit(EXIT_FAILURE);
    }

    grist_feature* f_ = NULL;
    grist_rev rev_;

    if(k[0] == '\0') {
        srand(time(NULL));
        int r = 0;
        do {
            r = rand();
            grist_md5hash(&r, sizeof(r), k);
            k[48] = '\0';
        } while((f_ = grist_db_get(db, k, strlen(k), &rev_)));
        if(f_) grist_feature_del(f_);
        f_ = NULL;
        report(GRISTMGR_INF, "generated key: %s", k);
        rev_.i = 1;
    } else {
        f_ = grist_db_get(db, k, strlen(k), &rev_);
        if(f_) {
            rev_.i++;
            grist_feature_del(f_);
            f_ = NULL;
        } else rev_.i = 1;
    }

    GEOSWKTReader* r = GEOSWKTReader_create();
    GEOSGeometry* g = GEOSWKTReader_read(r, wkt);

    if(!g) {
        report(GRISTMGR_ERR, "invalid geometry");
        exit(EXIT_FAILURE);
    }

    grist_feature* f = grist_feature_new();
    f->geom = g;
    f->attr = json_tokener_parse(attrs);

    if(!f->attr) {
        report(GRISTMGR_ERR, "could not parse attrs: %s", attrs);
        grist_db_close(db);
        grist_db_del(db);
        exit(EXIT_FAILURE);
    }

    if(!grist_db_put(db, k, strlen(k), f, &rev_)) {
        report(GRISTMGR_ERR, "could not update %s: %s", k, grist_db_errmsg(db));
        grist_db_close(db);
        grist_db_del(db);
        exit(EXIT_FAILURE);
    }

    grist_db_close(db);
    grist_db_del(db);
    db = NULL;

    report(GRISTMGR_INF, "ok.");
    return 0;

    opterr:
        usage();
        return EXIT_FAILURE;
}

static int doget(int argc, const char** argv) {
    
    if(argc<3) goto opterr;

    const char* fname = argv[2];
    const char* k = argv[3];

    if(!k) {
        report(GRISTMGR_ERR, "missing key");
        exit(EXIT_FAILURE);
    }
    
    grist_db* db = grist_db_new();
    if(!grist_db_open(db, fname, BDBOREADER)) {
        abort();
    }

    grist_rev r;
    grist_feature* f = grist_db_get(db, k, strlen(k), &r);
    if(!f) {
        abort();
    }

    char* fjson = grist_db_feature2json(f, &r, (void*)k, strlen(k));

    printf("%s\n", fjson);

    return 0;

    opterr:
        usage();
        return EXIT_FAILURE;
}

static int dolist(int argc, const char** argv) {
    if(argc<2) goto opterr;

    const char* fname = argv[2];
    
    grist_db* db = grist_db_new();
    if(!grist_db_open(db, fname, BDBOREADER)) {
        abort();
    }

    if(!grist_db_fcount(db)) {
        report(GRISTMGR_INF, "empty db.");
        return EXIT_SUCCESS;
    }

    BDBCUR* cur = grist_db_curnew(db);
    if(!cur) {
        abort();
    }

    int ksz;
    char* k;
    int i = 0;
    //grist_feature* f;
    do {
        k = grist_db_curkey(cur, &ksz);
        if(!k) break;
        //f = grist_db_get(db, k, ksz);
        //fjson = grist_feature_tojson(f);
        //printf("%s\t%s\n", k, fjson);
        //free(fjson);
        //fjson = NULL;
        //grist_feature_del(f);
        //f = NULL;
        //todo: optional doc printing
        printf("%s\n", k);
        free(k);
        k = NULL;
        ++i;
    } while(grist_db_curnext(cur));

    grist_db_close(db);
    grist_db_del(db);

    return 0;

    opterr:
        usage();
        return EXIT_FAILURE;
}

static int doeval(int argc, const char** argv) {
    if(argc<5) goto opterr;

    const char* fname = argv[2];
    const char* key = argv[3];
    const char* scriptnm = argv[4];

    grist_db* db = grist_db_new();
    if(!grist_db_open(db, fname, BDBOREADER)) {
        abort();
    }

    grist_db_jsinit(db);

    FILE* sfp = fopen(scriptnm, "rb");
    if(!sfp) {
        report(GRISTMGR_ERR, "could not open script: %s", scriptnm);
        exit(EXIT_FAILURE);
    }
    fseek(sfp, 0, SEEK_END);
    size_t flen = ftell(sfp);
    fseek(sfp, 0, SEEK_SET);
    char* script = malloc(flen+1);

    fread(script, 1, flen, sfp);
    script[flen] = '\0';

    if(!grist_db_jsload(db, script, strlen(script), NULL)) {
        report(GRISTMGR_ERR, "could not load script");
        goto opterr;
    }
    free(script);
    fclose(sfp);

    int sz;
    char* rjson = grist_db_jscalljson(db, "map", (void*)key, strlen(key), &sz);
    if(!rjson) {
        report(GRISTMGR_ERR, "could not evaluate script");
        goto opterr;
    }

    printf("%s\n", rjson);

    free(rjson);

    grist_db_del(db);

    return EXIT_SUCCESS;
    
    opterr:
        usage();
        return EXIT_FAILURE;
}

static int domap(int argc, const char** argv) {
    if(argc<5) goto opterr;

    const char* fname = argv[2];
    const char* scriptnm = argv[3];
    const char* outnm = argv[4];

    grist_db* db = grist_db_new();
    if(!grist_db_open(db, fname, BDBOREADER)) {
        abort();
    }

    grist_db_jsinit(db);

    FILE* sfp = fopen(scriptnm, "rb");
    if(!sfp) abort();
    fseek(sfp, 0, SEEK_END);
    size_t flen = ftell(sfp);
    fseek(sfp, 0, SEEK_SET);
    char* script = malloc(flen+1);

    fread(script, 1, flen, sfp);
    script[flen] = '\0';

    if(!grist_db_jsload(db, script, strlen(script), NULL)) {
        report(GRISTMGR_ERR, "could not load script");
        goto opterr;
    }
    free(script);

    int sz;
    BDBCUR* cur = grist_db_curnew(db);
    if(!cur) {
        abort();
    }

    grist_db* outdb = grist_db_new();
    if(!grist_db_open(outdb, outnm, BDBOWRITER|BDBOCREAT)) {
        report(GRISTMGR_ERR, "could not open output db: %s\n", outnm);
        exit(EXIT_FAILURE);
    }

    char* rjson;
    int ksz;
    char* k;
    int i = 0;
    grist_rev rev;
    rev.i = 1;
    rev.s[0] = '\0';
    grist_feature* f = NULL;
    char* jkey = NULL;
    do {
        k = grist_db_curkey(cur, &ksz);
        if(!k) break;
        rjson = grist_db_jscalljson(db, "map", (void*)k, strlen(k), &sz);
        if(!rjson) {
            report(GRISTMGR_ERR, "could not evaluate script");
            goto opterr;
        }
        json_object* rjsonobj = json_tokener_parse(rjson);
        if(rjsonobj && json_object_is_type(rjsonobj, json_type_object)) {
            json_object_object_foreach(rjsonobj, key, val) {
                if(!json_object_is_type(val, json_type_object)) continue;
                if(!(jkey = realloc(jkey, strlen(key)+1))) abort();
                strcpy(jkey, key);
                jkey[strlen(key)] = '\0';
                f = grist_feature_fromjson(json_object_to_json_string(val));
                if(f) {
                    grist_db_put(outdb, jkey, strlen(jkey), f, &rev);
                    grist_feature_del(f);
                    f = NULL;
                }
            }
        }
        //printf("%s\n", rjson);
        free(k);
        k = NULL;
        free(rjson);
        rjson = NULL;
        ++i;
    } while(grist_db_curnext(cur));

    grist_db_del(outdb);

    grist_db_del(db);

    return EXIT_SUCCESS;
    
    opterr:
        usage();
        return EXIT_FAILURE;
}

static int doload(int argc, const char** argv) {
    if(argc<4) goto opterr;

    const char* fname = argv[2];
    const char* inname = argv[3];

    FILE* dfp = fopen(inname, "rb");
    if(!dfp) {
        report(GRISTMGR_ERR, "could not open data file: %s", inname);
        exit(EXIT_FAILURE);
    }
    fseek(dfp, 0, SEEK_END);
    size_t flen = ftell(dfp);
    fseek(dfp, 0, SEEK_SET);
    char* data = malloc(flen+1);

    fread(data, 1, flen, dfp);
    data[flen] = '\0';

    grist_db* db = grist_db_new();
    if(!grist_db_open(db, fname, BDBOWRITER)) {
        report(GRISTMGR_ERR, "could not open db: %s", fname);
        goto opterr;
    }

    json_object* jso = json_tokener_parse(data);

    grist_feature* f = NULL;
    grist_rev r;
    r.i = 1;

    if(jso && json_object_is_type(jso, json_type_object)) {
        json_object_object_foreach(jso, key, val) {
            f = grist_feature_fromjson(json_object_to_json_string(val));
            if(f) {
                grist_db_put(db, key, strlen(key), f, &r);    
                grist_feature_del(f);
            }
            f = NULL;
        }
    } else {
        report(GRISTMGR_ERR, "could not parse json");
        goto opterr;
    }


    free(data);
    grist_db_del(db);

    return EXIT_SUCCESS;

    opterr:
        usage();
        return EXIT_FAILURE;
}
