#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <json/json.h>
#include <js/jsapi.h>
#include "db.h"
#include "feature.h"

grist_db* grist_db_new(void) {
    grist_db* db = malloc(sizeof(grist_db));
    db->bdb = tcbdbnew();
    db->jsrt = NULL;
    db->jscx = NULL;
    db->jsglob = NULL;
    return db;
}

bool grist_db_open(grist_db* db, const char* fname, int omode) {
    assert(tcbdbtune(db->bdb, -1, -1, -1, -1, -1, BDBTLARGE|BDBTBZIP));
    return tcbdbopen(db->bdb, fname, omode);
}

bool grist_db_close(grist_db* db) {
    return tcbdbclose(db->bdb);
}

void grist_db_del(grist_db* db) {
    assert(db);
    tcbdbdel(db->bdb);
    if(db->jscx) JS_DestroyContext(db->jscx);
    if(db->jsrt) JS_DestroyRuntime(db->jsrt);
    free(db);
    db = NULL;
    return;
}

bool grist_db_put(grist_db* db, const void* kbuf, int ksiz, grist_feature* f, grist_rev* r) {

    int packedsz = 0;
    char* packed = grist_db_packrec(db, f, r, &packedsz);
    if(!packed || !packedsz) {
        return false;
    }

    if(!tcbdbput(db->bdb, kbuf, ksiz, packed, packedsz)) {
        return false;
    }

    return true;
}

grist_feature* grist_db_get(grist_db* db, const void* kbuf, int ksiz, grist_rev* r) {
    
    int vsz;

    void* v = tcbdbget(db->bdb, kbuf, ksiz, &vsz);
    if(!v) {
        return NULL;
    }

    grist_feature* f = grist_db_unpackrec(db, v, vsz, r);

    free(v);
    return f;
}

void* grist_db_packrec(grist_db* db, grist_feature* f, grist_rev* rev, int* sz) {
    assert(db && f && f->geom && f->attr && sz && rev);
    int pfsz;
    void* pf = grist_feature_pack(f, &pfsz);
    assert(pf);
    grist_md5hash(pf, pfsz, rev->s);
    *sz = pfsz + GRIST_DB_REVSZ + sizeof(uint64_t);
    void* pr = malloc(*sz);
    uint64_t rev_i_ = htonll(rev->i);
    memcpy(pr, &rev_i_, sizeof(uint64_t));
    memcpy(pr+sizeof(uint64_t), rev->s, GRIST_DB_REVSZ);
    memcpy(pr+sizeof(uint64_t)+GRIST_DB_REVSZ, pf, pfsz);
    free(pf);
    return pr;
}

grist_feature* grist_db_unpackrec(grist_db* db, void* v, int vsz, grist_rev* rev) {
    assert(db && v && vsz > (GRIST_DB_REVSZ+3*sizeof(uint64_t)));
    uint64_t r_i;
    memcpy(&r_i, v, sizeof(uint64_t));
    rev->i = ntohll(r_i);
    strncpy(rev->s, v+sizeof(uint64_t), GRIST_DB_REVSZ);
    return grist_feature_unpack(v+sizeof(uint64_t)+GRIST_DB_REVSZ, vsz);
}

BDBCUR* grist_db_curnew(grist_db* db) {
    BDBCUR* cur = tcbdbcurnew(db->bdb);
    assert(cur);
    tcbdbcurfirst(cur);
    return cur;
}

bool grist_db_curnext(BDBCUR* cur) {
    return tcbdbcurnext(cur);
}

void* grist_db_curkey(BDBCUR* cur, int* szp) {
    return tcbdbcurkey(cur, szp);
}

uint64_t grist_db_fcount(grist_db* db) {
    return tcbdbrnum(db->bdb);
}

uint64_t grist_db_filesz(grist_db* db) {
    return tcbdbfsiz(db->bdb);
}

const char* grist_db_errmsg(grist_db* db) {
    int ecode = tcbdbecode(db->bdb);
    if(!ecode) return NULL;
    return tcbdberrmsg(ecode);
}

static JSClass global_class = {
    "global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

bool grist_db_jsinit(grist_db* db) {
    assert(!db->jsrt && !db->jscx && !db->jsglob);
    db->jsrt = JS_NewRuntime(8L * 1024L * 1024L);
    if(!db->jsrt) return false;
    db->jscx = JS_NewContext(db->jsrt, 8192);
    if(!db->jscx) return false;
    db->jsglob = JS_NewCompartmentAndGlobalObject(db->jscx, &global_class, NULL);
    if(!db->jsglob) return false;
    if(!JS_InitStandardClasses(db->jscx, db->jsglob)) return false;
    return true;
}

bool grist_db_jsload(grist_db* db, const char* src, int srcsz, jsval* rval) {
    JSBool ok = JS_EvaluateScript(db->jscx, db->jsglob, src, srcsz, "<gristdb>", 0, rval);
    return ok ? true : false;
}

typedef struct grist_db_jsonwriter_s {
    grist_db* db;
    char* buf;
} grist_db_jsonwriter;

JSBool _grist_db_jsonwritecb(const jschar *buf, uint32 len, void* data) {
    grist_db_jsonwriter* w = (grist_db_jsonwriter*)data;
    grist_db* db = w->db;
    if(!db || w->buf) return JS_FALSE;
    JSString* jsstr = JS_NewUCStringCopyN(db->jscx, buf, len);
    size_t elen = JS_GetStringEncodingLength(db->jscx, jsstr);
    char* wbuf = malloc(elen+1);
    JS_EncodeStringToBuffer(jsstr, wbuf, len);
    wbuf[elen] = '\0';
    w->buf = wbuf;
    return JS_TRUE;
}

jsval* grist_db_jscall(grist_db* db, const char* fxn, void* k, int ksz) {
    assert(db->jsrt && db->jscx && db->jsglob);
    grist_rev r;
    grist_feature* f = grist_db_get(db, k, ksz, &r);
    if(!f) return NULL;
    //char* fjson = grist_feature_tojson(f);
    char* fjson = grist_db_feature2json(f, &r, k, ksz); 
    jsval parsed;
    JSONParser* jp = JS_BeginJSONParse(db->jscx, &parsed);
    assert(jp);
    JSString* fjsonjs = JS_NewStringCopyN(db->jscx, fjson, strlen(fjson));
    JSBool ok = JS_ConsumeJSONText(db->jscx, jp, 
        JS_GetStringCharsZ(db->jscx, fjsonjs), JS_GetStringLength(fjsonjs));
    JS_FinishJSONParse(db->jscx, jp, JSVAL_NULL);
    jsval* rval = malloc(sizeof(jsval));
    *rval = JSVAL_NULL;
    ok = JS_CallFunctionName(db->jscx, db->jsglob, fxn, 1, &parsed, rval);
    if(!ok) return NULL;
    grist_feature_del(f);
    free(fjson);
    return rval;
}

char* grist_db_jscalljson(grist_db* db, const char* fxn, void* k, int ksz, int* jsz) {
    jsval* rval = grist_db_jscall(db, fxn, k, ksz);
    if(!rval) return NULL;
    grist_db_jsonwriter writer;
    writer.db = db;
    writer.buf = NULL;
    JSBool ok = JS_Stringify(db->jscx, rval, NULL, JSVAL_ONE, _grist_db_jsonwritecb, &writer);
    if(!ok) return NULL;
    *jsz = strlen(writer.buf);
    free(rval);
    return writer.buf;
}

char* grist_db_feature2json(grist_db* db, grist_feature* f, grist_rev* r, void* key, int ksz) {
    GEOSWKTWriter* w = GEOSWKTWriter_create();
    const char* wkt = GEOSWKTWriter_write(w, f->geom);
    GEOSWKTWriter_destroy(w);
    JS_Object* jso = JS_NewObject(db->context, NULL, NULL, NULL);
    assert(JSVAL_IS_OBJECT(jso));
    JSString* jwkt = JS_NewStringCopyN(db->jscx, wkt, strlen(wkt));
    JS_SetProperty(db->jscx, jso, "geom", jwkt);
    JS_SetProperty(db->jscx, jso, "attr", jwkt);
    size_t bufsz = 60;
    char buf[bufsz];
    snprintf(buf, bufsz, "%lld-%s", (long long int)r->i, r->s);
    buf[bufsz-1] = '\0';
    json_object* rstr = json_object_new_string(buf);
    json_object_object_add(fjsobj, "_rev", rstr);
    if(key && ksz > 0) {
        char* kstr = malloc(ksz+1);
        strncpy(kstr, key, ksz);
        kstr[ksz] = '\0';
        json_object* kjstr = json_object_new_string(kstr);
        free(kstr);
        json_object_object_add(fjsobj, "_key", kjstr);
    }
    return (char*)json_object_to_json_string(fjsobj);
}

int grist_db_parsejson(grist_db* db, char* s, int sz, jsval *v) {
    assert(db && db->jscx && v);
    JSString* jstr = JS_NewStringCopyN(db->jscx, s, sz);
    JSONParser* jp = JS_BeginJSONParse(db->jscx, v);
    JSBool ok = JS_ConsumeJSONText(db->jscx, jp, 
        JS_GetStringCharsZ(db->jscx, jstr), JS_GetStringLength(jstr));
    JS_FinishJSONParse(db->jscx, jp, JSVAL_NULL);
    if(!ok) {
        v = NULL;
        return -1;
    }
    return 0;
}
