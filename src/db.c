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
    db->jsruntime = NULL;
    db->jscontext = NULL;
    db->jsglobal = NULL;
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
    if(db->jscontext) JS_DestroyContext(db->jscontext);
    if(db->jsruntime) JS_DestroyRuntime(db->jsruntime);
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
    assert(db && f && sz && rev);
    int pfsz;
    void* pf = grist_feature_pack(f, &pfsz);
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
    assert(!db->jsruntime && !db->jscontext && !db->jsglobal);
    db->jsruntime = JS_NewRuntime(8L * 1024L * 1024L);
    if(!db->jsruntime) return false;
    db->jscontext = JS_NewContext(db->jsruntime, 8192);
    if(!db->jscontext) return false;
    db->jsglobal = JS_NewCompartmentAndGlobalObject(db->jscontext, &global_class, NULL);
    if(!db->jsglobal) return false;
    if(!JS_InitStandardClasses(db->jscontext, db->jsglobal)) return false;
    return true;
}

bool grist_db_jsload(grist_db* db, const char* src, int srcsz, jsval* rval) {
    JSBool ok = JS_EvaluateScript(db->jscontext, db->jsglobal, src, srcsz, "<gristdb>", 0, rval);
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
    JSString* jsstr = JS_NewUCStringCopyN(db->jscontext, buf, len);
    size_t elen = JS_GetStringEncodingLength(db->jscontext, jsstr);
    char* wbuf = malloc(elen+1);
    JS_EncodeStringToBuffer(jsstr, wbuf, len);
    wbuf[elen] = '\0';
    w->buf = wbuf;
    return JS_TRUE;
}

char* grist_db_jscall(grist_db* db, const char* fxn, void* k, int ksz, int* rvsz) {
    assert(db->jsruntime && db->jscontext && db->jsglobal);
    grist_rev r;
    grist_feature* f = grist_db_get(db, k, ksz, &r);
    if(!f) return NULL;
    const char* fjson = grist_feature_tojson(f);
    jsval parsed;
    JSONParser* jp = JS_BeginJSONParse(db->jscontext, &parsed);
    assert(jp);
    JSString* fjsonjs = JS_NewStringCopyN(db->jscontext, fjson, strlen(fjson));
    JSBool ok = JS_ConsumeJSONText(db->jscontext, jp, 
        JS_GetStringCharsZ(db->jscontext, fjsonjs), JS_GetStringLength(fjsonjs));
    JS_FinishJSONParse(db->jscontext, jp, JSVAL_NULL);
    jsval rval = JSVAL_NULL;
    ok = JS_CallFunctionName(db->jscontext, db->jsglobal, fxn, 1, &parsed, &rval);
    if(!ok) return NULL;
    grist_db_jsonwriter writer;
    writer.db = db;
    writer.buf = NULL;
    ok = JS_Stringify(db->jscontext, &rval, NULL, JSVAL_ONE, _grist_db_jsonwritecb, &writer);
    if(!ok) return NULL;
    *rvsz = strlen(writer.buf);
    grist_feature_del(f);
    return writer.buf;
}

const char* grist_db_feature2json(grist_feature* f, grist_rev* r) {
    json_object* fjsobj = json_object_new_object();
    GEOSWKTWriter* w = GEOSWKTWriter_create();
    const char* wkt = GEOSWKTWriter_write(w, f->geom); // todo: geojson here.
    json_object* gstr = json_object_new_string(wkt);
    json_object_object_add(fjsobj, "geom", gstr);
    json_object_object_add(fjsobj, "attr", f->data);
    size_t bufsz = 60;
    char buf[bufsz];
    snprintf(buf, bufsz, "%lld-%s", (long long int)r->i, r->s);
    buf[bufsz-1] = '\0';
    json_object* rstr = json_object_new_string(buf);
    json_object_object_add(fjsobj, "_rev", rstr);
    return json_object_to_json_string(fjsobj);
}
