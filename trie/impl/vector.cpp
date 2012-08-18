#pragma once
#include <vector>
#include <algorithm>
#include <utility>
#include <functional>
#include <cstddef>
#include "base.h"
#include "../../util/containers.h"

// iter_swap trick only works on gcc

template<class T, class K, class tails = std::vector< std::pair<K,T*> > >
struct Vector_base : protected tails {
    typedef T* pointer;
    typedef T& reference;

    typedef typename tails::value_type value_type;
    typedef typename tails::iterator iterator;

    Vector_base() : tails() { }

    pointer find_node(const char* str, size_t& ofs, bool opt=true)
    {
	iterator p = seek_node(str[ofs],opt);
	if(p != tails::end() && key_traits<K>::match_key(p->first, str, ofs))
	    return p->second;
	else
	    return 0;
    }

    iterator seek_node(const char k, bool opt=true) 
    {
        for(iterator p = tails::begin(); p != tails::end(); ++p) 
            if(p->first == k) {
	    #if BUBBLE
		typename tails::iterator q = p;
                if(opt && p != tails::begin()) iter_swap(--p, q);
	    #else
                if(opt) iter_swap(p, tails::begin()), p = tails::begin();
	    #endif
		return p;
            }
        return tails::end();
    }

    void attach_node(K ch, pointer p)
    {
	tails::push_back(value_type(ch,p));
    }

    reference select_node(const char* str, size_t ofs=0)
    {
//if(ofs==0) puts(str);
	const size_t begin_ofs = ofs;
	const iterator p = seek_node(str[ofs]);
	if(p == tails::end()) {
//printf("%d>create\n", ofs);
	    K ch = key_traits<K>::extract_key(str,ofs);
	    pointer ptr;
	    reference rn = T::create(ptr,str,ofs);
	    attach_node(ch, ptr);
	    return rn;
	} else if(key_traits<K>::match_key(p->first, str, ofs)) {
//printf("%d>insert %s %d\n", ofs, p->first.c_str(), ofs-begin_ofs);
	    return p->second->insert(str,ofs);
	} else {
//printf("%d>split %s %d\n", ofs, p->first.c_str(), ofs-begin_ofs);
	    return *key_traits<K>::split_key(p->second, p->first, ofs-begin_ofs, str, ofs);
	}
    }

    using tails::reserve;
    using tails::empty;
    using tails::capacity;

    size_t arity() const
    {
	return tails::size();
    }

    std::pair<K,pointer> successor() const
    {
	return arity()==1? (*this)[0] : std::make_pair(K(),0);
    }

    /* utilities */
    T* this_T() 
    {
	return static_cast<T*>(this);
    }

    template<class F>
    void explore(F fun, bool=0)
    {
	for(iterator p = tails::begin(); p != tails::end(); ++p)
	    fun(p->first, *p->second);
    }

    template<class F>
    void walk(F fun, const size_t lvl=0, K k=K()) 
    {
	if(fun(k,*this_T(),lvl))
	    for(iterator p = tails::begin(); p != tails::end(); ++p)
		p->second->walk<F>(fun,lvl+1,p->first);
    }

    size_t optimize()
    {
	std::vector<std::pair<size_t, value_type> > n;
	size_t acc = !!this_T()->search_key;
	n.reserve(arity());

	for(iterator p = tails::begin(); p != tails::end(); ++p) {
	    size_t card = p->second->optimize();
	    n.push_back(std::make_pair(card,*p));
	    acc += card;
	}

	std::sort(n.begin(), n.end(), onkey(&std::pair<size_t,value_type>::first, std::greater<size_t>()));

	for(size_t i=0; i < n.size(); ++i) (*this)[i] = n[i].second;
	return acc;
    }
};

template<class T,class K>
struct Vector : Vector_base<T,K> {
    typedef Vector link;
};


template<class T,class K> 
struct CompactVector;

template<class T>
struct CompactVector<T,char> : Vector_base<T,char,compact_association_vector<T> > {
    typedef CompactVector link;
};


template<class T, class K>
struct SortedVector : Vector<T,K> {
    typedef SortedVector link;
    typedef typename Vector<T,K>::iterator iterator;
    typedef typename Vector<T,K>::pointer pointer;
    typedef typename Vector<T,K>::reference reference;
    typedef typename Vector<T,K>::value_type value_type;

    bool index(char ch, iterator& pos)
    {
	iterator begin = Vector<T,K>::begin();
	iterator end   = Vector<T,K>::end();
	if(begin != end) {
volatile value_type _ = *begin;
}
	while(begin != end) {
	    iterator const mid = begin+(end-begin)/2;
	    if(ch < mid->first) end = mid; else
	    if(ch > mid->first) begin = mid+1; else
	    return pos=mid, true;
	}
	return pos=begin, false;
    }

    pointer find_node(const char* str, size_t& ofs, bool=false)
    {
	iterator pos;
	if(index(str[ofs], pos) && key_traits<K>::match_key(pos->first, str, ofs))
	    return pos->second;
	else
	    return 0;
    }

    void attach_node(K ch, pointer p)
    {
	iterator pos;
	index(ch,pos);
	Vector<T,K>::insert(pos, value_type(ch,p));
    }

    reference select_node(const char* str, size_t ofs=0)
    {
	const size_t begin_ofs = ofs;
	iterator pos;
	if(index(str[ofs],pos))
	    if(key_traits<K>::match_key(pos->first, str, ofs))
		return pos->second->insert(str, ofs);
	    else 
		return *key_traits<K>::split_key(pos->second, pos->first, ofs-begin_ofs, str, ofs);
	else {
	    K ch = key_traits<K>::extract_key(str, ofs);
	    pointer tmp;
	    reference rn = T::create(tmp,str,ofs);
	    Vector<T,K>::insert(pos, value_type(ch,tmp));
	    return rn;
	}
    }
private:
    void optimize();
};

//////////////////////////////////

template<class T, class K>
struct IndirectVector : private std::vector<T*> {
    typedef IndirectVector link;
    typedef T* pointer;
    typedef T& reference;
    typedef std::vector<T*> tails;

    K key;

    typedef typename std::vector<T*>::value_type value_type;
    typedef typename std::vector<T*>::iterator iterator;

    IndirectVector() : tails() { }

    pointer find_node(const char* str, size_t& ofs, bool opt=true)
    {
	iterator p = seek_node(str[ofs],opt);
	if(p != tails::end() && key_traits<K>::match_key((*p)->key, str, ofs))
	    return *p;
	else
	    return 0;
    }

    iterator seek_node(const char k, bool opt=true) 
    {
        for(iterator p = tails::begin(); p != tails::end(); ++p) 
            if((*p)->key == k) {
	    #if BUBBLE
		typename tails::iterator q = p;
                if(opt && p != tails::begin()) iter_swap(--p, q);
	    #else
                if(opt) iter_swap(p, tails::begin()), p = tails::begin();
	    #endif
		return p;
            }
        return tails::end();
    }

    void attach_node(K ch, pointer p)
    {
	p->key = ch;
	tails::push_back(p);
    }

    reference select_node(const char* str, size_t ofs=0)
    {
//if(ofs==0) puts(str);
	const size_t begin_ofs = ofs;
	const iterator p = seek_node(str[ofs]);
	if(p == tails::end()) {
//printf("%d>create\n", ofs);
	    K ch = key_traits<K>::extract_key(str,ofs);
	    pointer ptr;
	    reference rn = T::create(ptr,str,ofs);
	    attach_node(ch, ptr);
	    return rn;
	} else if(key_traits<K>::match_key((*p)->key, str, ofs)) {
//printf("%d>insert %s %d\n", ofs, p->first.c_str(), ofs-begin_ofs);
	    return (*p)->insert(str,ofs);
	} else {
//printf("%d>split %s %d\n", ofs, p->first.c_str(), ofs-begin_ofs);
	    return *key_traits<K>::split_key(*p, (*p)->key, ofs-begin_ofs, str, ofs);
	}
    }

    using tails::reserve;
    using tails::empty;
    using tails::capacity;

    size_t arity() const
    {
	return tails::size();
    }

    std::pair<K,pointer> successor() const
    {
	return arity()==1? (*this)[0] : std::make_pair(K(),0);
    }

    /* utilities */
    T* this_T() 
    {
	return static_cast<T*>(this);
    }

    template<class F>
    void explore(F fun, bool=0)
    {
	for(iterator p = tails::begin(); p != tails::end(); ++p)
	    fun((*p)->key, **p);
    }

    template<class F>
    void walk(F fun, const size_t lvl=0, K k=K()) 
    {
	if(fun(k,*this_T(),lvl))
	    for(iterator p = tails::begin(); p != tails::end(); ++p)
		(*p)->walk<F>(fun,lvl+1,(*p)->key);
    }

    size_t optimize()
    {
	std::vector<std::pair<size_t, value_type> > n;
	size_t acc = !!this_T()->search_key;
	n.reserve(arity());

	for(iterator p = tails::begin(); p != tails::end(); ++p) {
	    size_t card = (*p)->optimize();
	    n.push_back(std::make_pair(card,*p));
	    acc += card;
	}

	std::sort(n.begin(), n.end(), onkey(&std::pair<size_t,value_type>::first, std::greater<size_t>()));

	for(size_t i=0; i < n.size(); ++i) (*this)[i] = n[i].second;
	return acc;
    }
};
