#include "environ.h"
#include <sys/times.h>
#include <time.h>
#include <stdio.h>

// link with librt
#define REALTIME

const char* tstamp()
{
    static char tmp[64];
#ifndef REALTIME
    tms t;
    times(&t);
    static tms p(t);
    sprintf(tmp, "[+%6.3f]",(t.tms_utime-p.tms_utime)/100.0);
#else
    timespec t;
    clockid_t id = CLOCK_REALTIME;
    //clock_getres(id, &t);
    //unsigned long long res = t.tv_sec*1000000ULL + t.tv_nsec;
    //printf("RES: %lld\n", res);
    clock_gettime(id, &t);
    static timespec p(t);
    sprintf(tmp, "[+%9.3f]",((t.tv_sec-p.tv_sec)*1e9 + t.tv_nsec - p.tv_nsec)/1e6);
#endif
    p = t;
    return tmp;
}

size_t allocated(size_t size)
{
    static const size_t ptr_siz = sizeof(void*);
    return size? std::max(4*ptr_siz, (size+3*ptr_siz-1)&~(2*ptr_siz-1)) : size;
}
