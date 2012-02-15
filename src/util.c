#include <stdint.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <tcutil.h>

int is_big_endian(void) {
    static int big_endian = -1;
    if(big_endian < 0) {
        union {
            uint64_t ull;
            uint8_t b[8];
        } checkme;
        checkme.ull = 0x01;
        if(checkme.b[7] == 0x01)
            big_endian = 1;
        else big_endian = 0;
    }
    return big_endian;
}

uint64_t htonll(uint64_t v) {
    uint64_t sw;
    if(is_big_endian()) {
        sw = v;
    } else {
        union {
            uint64_t ull;
            uint8_t b[8];
        } u;
        u.ull = v;
        uint8_t t;
        int i;
        for(i=0; i<4; i++) {
            t = u.b[i];
            u.b[i] = u.b[7-i];
            u.b[7-i] = t;
        }
        sw = u.ull;
    }
    return sw;
}

uint64_t ntohll(uint64_t v) {
    return htonll(v);
}

uint64_t dtoll(double d) {
    union {
        double d;
        uint64_t ull;
    } u;
    u.d = d;
    return u.ull;
}

double lltod(uint64_t ll) {
    union {
        double d;
        uint64_t ull;
    } u;
    u.ull = ll;
    return u.d;
}

void grist_md5hash(const void* p, int sz, char* buf) {
    return tcmd5hash(p, sz, buf);
}
