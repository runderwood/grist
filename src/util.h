#ifndef GRIST_UTIL_H
#define GRIST_UTIL_H
int is_big_endian(void);
uint64_t htonll(uint64_t v);
uint64_t ntohll(uint64_t v);
uint64_t dtoll(double d);
double lltod(uint64_t ll);
#endif

