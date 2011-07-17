#include <cmath>
#include "../environ.h"
#include "fnv.h"

#ifndef BITS
#define BITS 16
#endif
template<class T>
struct hash_table {
    typedef std::pair<char*, T> value_type;
    typedef typename std::vector<value_type>::iterator iterator;
    typedef typename std::vector<value_type> bucket;
    std::vector<bucket> tab;
    size_t count;

    void inflate()
    {
	int const maxsize = log(0.05)/(log(tab.size()-1)-log(tab.size()));
	//int const maxsize = log(tab.size())*tab.size();
	if(count <= maxsize) return;
	std::vector<bucket> ntab(tab.size()*2);
	for(size_t h = 0; h < tab.size(); ++h)
	    for(size_t i = 0; i < tab[h].size(); ++i) {
		unsigned long const nh = fnv::hash32(tab[h][i].first) & (2*tab.size()-1);
		ntab[nh].push_back(tab[h][i]);
	    }
	ntab.swap(tab);
    }

    const value_type* search(const char* str)
    {
	unsigned long const h = fnv::hash32(str) & tab.size()-1;
	for(iterator p = tab[h].begin(); p != tab[h].end(); ++p)
	    if(strcmp(str, p->first) == 0) return &*p;
	return 0;
    }

    void insert(const char* str, const T& val)
    {
	if(!search(str)) {
	    ++count;
	    inflate();
	    unsigned long const h = fnv::hash32(str) & tab.size()-1;
	    tab[h].push_back(value_type(strcpy(new char[strlen(str)+1], str), val));
	} 
    }

    size_t buckets() const 
    { return tab.size(); }

    hash_table() : tab(1UL<<BITS), count() { }
};

template<class T>
size_t memused(hash_table<T>& lex)
{
    size_t acc = allocated(sizeof lex) + allocated(lex.tab.capacity() * sizeof(typename hash_table<T>::bucket));
    //size_t acc = allocated(sizeof lex) + allocated(lex.tab.size() * sizeof(typename hash_table<T>::bucket));
    for(int h=0; h < lex.buckets(); ++h) {
	acc += allocated(sizeof lex.tab[0] * lex.tab[h].capacity());
	for(int i=0; i < lex.tab[h].size(); ++i) {
	    acc += allocated(strlen(lex.tab[h][i].first)+1);
	}
    }
    return acc;
}
