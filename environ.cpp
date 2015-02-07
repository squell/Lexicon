#include "environ.h"
#include <sys/times.h>

const char* tstamp()
{
    tms t;
    times(&t);
    static tms p(t);
    static char tmp[64];
    std::sprintf(tmp, "[+%6.3f]",(t.tms_utime-p.tms_utime)/100.0);
    p = t;
    return tmp;
}

size_t allocated(size_t size)
{
    static const size_t ptr_siz = sizeof(void*);
    return size? std::max(4*ptr_siz, (size+3*ptr_siz-1)&~(2*ptr_siz-1)) : size;
}
