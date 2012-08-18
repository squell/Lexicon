#pragma once
#include <vector>
#include <algorithm>
#include <utility>
#include <functional>
#include <cstddef>
#include "base.h"

// iter_swap trick only works on gcc

template<class T, class K>
struct Array;

template<class T>
struct Array<T,char> {
    typedef Array link;
    typedef T* pointer;
    typedef T& reference;

    pointer tails[0x100];

    Array() : tails() { }

    pointer find_node(const char* str, size_t& ofs, bool opt=true)
    {
	return str[ofs++&0xFF][tails];
    }

    void attach_node(char ch, pointer p)
    {
	tails[ch&0xFF] = p;
    }

    reference select_node(const char* str, size_t ofs=0)
    {
	if(pointer p = find_node(str,ofs)) 
	    return p->insert(str,ofs);
	else {
	    reference rn = T::create(p,str,ofs);
	    attach_node(str[ofs-1], p);
	    return rn;
	}
    }

    void reserve(size_t) const { }

    size_t arity() const 
    {
	size_t acc = 0;
	for(int i=0; i < 256; i++) acc += !!tails[i];
	return acc;
    }

    size_t empty() const
    {
	for(int i=0; i < 256; i++) if(tails[i]) return false;
	return true;
    }

    std::pair<char,pointer> successor() const 
    {
	for(int i=0; i < 256; i++) if(tails[i]) {
	    for(int j=i+1; j < 256; j++)
		if(tails[j]) return std::make_pair(0, 0);
	    return std::make_pair(i, tails[i]);
	}
	return std::make_pair(0, 0);
    }

    /* utilities */
    T* this_T() 
    {
	return static_cast<T*>(this);
    }

    template<class F>
    void explore(F fun, bool=0)
    {
	for(char i=0; i < 256; i++) {
	    char c = i;
	    if(tails[i]) fun(c, *tails[i]);
	}
    }

    template<class F>
    void walk(F fun, const size_t lvl=0, char k=0) 
    {
	if(fun(k,*this_T(),lvl))
	    for(int i = 0; i < 256; ++i) {
		char c = i;
		if(tails[i]) tails[i]->walk<F>(fun,lvl+1,c);
	    }
    }

    void optimize()
    {
    }

};

template<class T, class K>
struct Array1;

template<class T>
struct Array1<T,char> {
    typedef Array1 link;
    typedef T* pointer;
    typedef T& reference;

    pointer tails[0x100];

    Array1() : tails() { }

    pointer find_node(const char* str, size_t& ofs, bool opt=true)
    {
	return str[ofs++&0xFF][tails];
    }

    void attach_node(char ch, pointer p)
    {
	tails[ch&0xFF] = p;
    }

    reference select_node(const char* str, size_t ofs=0)
    {
	if(pointer p = find_node(str,ofs)) 
	    return p->insert(str,ofs);
	else {
	    reference rn = T::create(p,str,ofs);
	    attach_node(str[ofs-1], p);
	    return rn;
	}
    }

    void reserve(size_t) const { }

    size_t arity() const 
    {
	size_t acc = 0;
	for(int i=0; i < 256; i++) acc += !!tails[i];
	return acc;
    }

    size_t empty() const
    {
	for(int i=0; i < 256; i++) if(tails[i]) return false;
	return true;
    }

    std::pair<char,pointer> successor() const 
    {
	for(int i=0; i < 256; i++) if(tails[i]) {
	    for(int j=i+1; j < 256; j++)
		if(tails[j]) return std::make_pair(0, 0);
	    return std::make_pair(i, tails[i]);
	}
	return std::make_pair(0, 0);
    }

    /* utilities */
    T* this_T() 
    {
	return static_cast<T*>(this);
    }

    template<class F>
    void explore(F fun, bool=0)
    {
	for(char i=0; i < 256; i++) {
	    char c = i;
	    fun(c, *tails[i]);
	}
    }

    template<class F>
    void walk(F fun, const size_t lvl=0, char k=0) 
    {
	if(fun(k,*this_T(),lvl))
	    for(int i = 0; i < 256; ++i) {
		char c = i;
		if(tails[i]) tails[i]->walk<F>(fun,lvl+1,c);
	    }
    }

};
