#pragma once
#include <vector>
#include <algorithm>
#include <utility>
#include <functional>
#include <cstddef>
#include "base.h"

template<class T, class K>
struct LinkedList {
    typedef T* pointer;
    typedef T& reference;
    typedef LinkedList link;

    pointer next, sib;
    K key;

    LinkedList() : key(), next(), sib() { }

#if BUBBLE
    pointer& seek_node(char ch, bool opt=true) 
    {
	pointer* prv = 0;
	pointer* cur = &next;
	while(*cur) {
	    if((*cur)->key == ch) 
		if(opt && prv) {
		    pointer p = *cur;
		    *cur = p->sib;
		    p->sib = *prv;
		    return *prv = p;
		} else 
		    break;
	    prv = cur;
	    cur = &(*cur)->sib;
	}
	return *cur;
    }
#else
    pointer& seek_node(char ch, bool opt=true) 
    {
	pointer* cur = &next;
	while(*cur) {
	    if((*cur)->key == ch) 
		if(opt) {
		    pointer p = *cur;
		    *cur = p->sib;
		    p->sib = next;
		    return next = p;
		} else 
		    break;
	    cur = &(*cur)->sib;
	}
	return *cur;
    }
#endif

    pointer find_node(const char* str, size_t& ofs, bool opt=true)
    {
	pointer p = seek_node(str[ofs], opt);
	if(p && key_traits<K>::match_key(p->key, str, ofs))
	    return p;
	else
	    return 0;
    }

    void attach_node(K k, pointer p)
    {
	p->key = k;
	p->sib = next;
	next = p;
    }

    reference select_node(const char* str, size_t ofs=0)
    {
	const size_t begin_ofs = ofs;
	pointer& p = seek_node(str[ofs]);
	if(p && key_traits<K>::match_key(p->key, str, ofs)) {
	    return p->insert(str, ofs);
	} else if(!p) {
	    K key = key_traits<K>::extract_key(str, ofs);
	    reference rn = T::create(p, str, ofs);
	    p->key = key;
	    return rn;
	} else {
	    pointer sib = p->sib;
	    K key = p->key;
	    pointer rn = key_traits<K>::split_key(p, key, ofs-begin_ofs, str, ofs);
	    p->sib = sib;
	    p->key = key;
	    return *rn;
	}
    }

    void reserve(size_t) const { }

    bool empty() const
    {
	return !next;
    }

    size_t arity() const
    {
	size_t acc = 0;
	for(pointer p = next; p; p = p->sib) ++acc;
	return acc;
    }

    std::pair<K,pointer> successor() const
    {
	return next&&!next->sib? std::make_pair(next->key, next) : std::make_pair(K(),0);
    }

    /* utilities */
    T* this_T() 
    {
	return static_cast<T*>(this);
    }

    template<class F>
    void explore(F fun, bool=0)
    {
	for(pointer p = next; p; p=p->sib)
	    fun(p->key, *p);
    }


    template<class F>
    void walk(F fun, const size_t ofs=0) 
    {
	if(fun(key,*this_T(),ofs))
	    for(pointer p = next; p; p=p->sib) 
		p->walk<F>(fun,ofs+1);
    }

    size_t optimize()
    {
	std::vector<std::pair<size_t, pointer> > n;
	size_t acc = !!this_T()->search_key;

	for(pointer p = next; p; p=p->sib) {
	    size_t card = p->optimize();
	    n.push_back(std::make_pair(card,p));
	    acc += card;
	}

	std::sort(n.begin(), n.end(), onkey(&std::pair<size_t,pointer>::first));
	std::reverse(n.begin(), n.end());

	next = 0;
	for(size_t i=0; i < n.size(); ++i) {
	    pointer& p = n[i].second;
	    p->sib = next;
	    next = p;
	}
	return acc;
    }

    void sort()
    {
	std::vector<std::pair<K, pointer> > n;

	for(pointer p = next; p; p=p->sib) {
	    p->sort();
	    n.push_back(std::pair<K,pointer>(key,p));
	}

	std::sort(n.begin(), n.end(), onkey(&std::pair<K,pointer>::first));

	pointer* p = &next;
	next = 0;
	for(size_t i=0; i < n.size(); ++i) {
	    *p = n[i].second;
	    p = &(*p)->sib;
	}
	*p = 0;
    }
};
