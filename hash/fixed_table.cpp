#include "../environ.h"
#include "fnv.h"

#ifndef BITS
#define BITS 16
#endif
template<class T, size_t bits = BITS, bool xor_fold = false>
struct hash_table {
    typedef std::pair<char*, T> value_type;
    typedef typename std::vector<value_type>::iterator iterator;
    std::vector<value_type> tab[1UL<<bits];

    static unsigned long hash(const char* str)
    {
	unsigned long const h = fnv::hash32(str);
	if(xor_fold)
	    return (h ^ h >> bits) & (1UL<<bits)-1;
	else
	    return h & (1UL<<bits)-1;
    }

    const value_type* search(const char* str)
    {
	unsigned long const h = hash(str);
	for(iterator p = tab[h].begin(); p != tab[h].end(); ++p)
	    if(strcmp(str, p->first) == 0) return &*p;
	return 0;
    }

    void insert(const char* str, const T& val)
    {
	unsigned long const h = hash(str);
	if(!search(str))
	    tab[h].push_back(value_type(strcpy(new char[strlen(str)+1], str), val));
    }

    static size_t buckets()
    { return 1UL<<bits; }
};


template<class T, size_t N, bool B>
size_t memused(hash_table<T,N,B>& lex)
{
    size_t acc = allocated(sizeof lex);
    for(int h=0; h < lex.buckets(); ++h) {
	acc += allocated(sizeof lex.tab[0] * lex.tab[h].capacity());
	for(int i=0; i < lex.tab[h].size(); ++i) {
	    acc += allocated(strlen(lex.tab[h][i].first)+1);
	}
    }
    return acc;
}
