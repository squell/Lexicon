/*

Fowler-Noll-Vo hash functions (32-bit and 64-bit)

Copyright (c) 2012 Marc Schoolderman

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.

*/

#ifndef INCLUDED_FNV_HASH
#define INCLUDED_FNV_HASH
#include <stdint.h>

namespace fnv {

inline uint32_t hash32(uint32_t h, char c)
{
    uint32_t const prime = 16777619UL;
    return (h^c)*prime;
}

inline uint32_t hash32(const char* str, uint32_t h = 2166136261UL)
{
    while(*str) h = hash32(h, *str++);
    return h;
}

inline uint64_t hash64(uint64_t h, char c)
{
    uint64_t const prime = 1099511628211ULL;
    return (h^c)*prime;
}

inline uint64_t hash64(const char* str, uint64_t h = 14695981039346656037ULL)
{
    while(*str) h = hash64(h, *str++);
    return h;
}

}
#endif
