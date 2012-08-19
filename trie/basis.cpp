#pragma once

#include <cstddef>
#include <cstring>
#include "../util/containers.h"
#include "../environ.h"
#include "impl/base.h"
#include "impl/vector.cpp"

// experimentally, we find 3 to be best.
#define DEMOTE 3
#define OPTIMIZE 0
#define BUBBLE 0

// 0 - strict trie (node is either node or leaf)
// 1 - only demote on conflict
// 2 - only demote on conflict, also claim node if available and no conflict
// 3 - always demote if we are shorter (digital search tree)
// 4 - tries to find the optimal way of demoting

/* a boxed value */

template<class T> struct value {
    typedef T& reference;
    T info;
    explicit value(const T& val = T()) : info(val) { }
    void set(const value& val = T()) { info = val.info; }

    const T* operator->() const { return &info; }
    T* operator->() { return &info; }
};

template<> struct value<void> { 
    typedef void reference;
    value(bool = 0) { }
    void set(value = 0) { }
};

/* the trie itself */

struct trie_storage {
    const char* search_key;
    trie_storage(const char* p=0) : search_key(p) { }
};

template<class T, template <class,class> class Link, bool Reduced = false, class Key = char>
struct trie : trie_storage, Link<trie<T,Link,Reduced,Key>, Key>, value<T> {
    typedef typename Link<trie,Key>::link link;
    typedef typename link::pointer pointer;
    typedef Key key_type;
    typedef trie trie_type;
    enum { full_key = !Reduced };

    trie() 
    : trie_storage() { }

    trie(const char* key, size_t ofs=0)
    : trie_storage(own_key(key,ofs)) { }

    static trie& create(trie*& node, const char* key, size_t ofs=0)
    {
	return *(node = new trie(key,ofs));
    }
      
    static const char* own_key(const char* key, size_t ofs=0) 
    {
	if(Reduced) key += ofs;
	return *key? std::strcpy(new char[std::strlen(key)+1], key) : "";
    }

    bool match_tail(const char* str, size_t i=0) const
    {
	const size_t ofs = Reduced? i : 0;
        do {
            if(search_key[i-ofs] != str[i]) return false;
        } while(str[i++]);
        return true;
    }

    const trie* search(const char* str) 
    {
        trie* cur_trie = this;
	size_t ofs = 0;
        do if(cur_trie->search_key && cur_trie->match_tail(str,ofs))
            return cur_trie;
        else
	    cur_trie = str[ofs]? cur_trie->find_node(str,ofs,OPTIMIZE) : 0;
        while(cur_trie);
	return 0;
    }

    bool shorter_than_tail(const char* str, size_t i=0) const
    {
	const size_t ofs = Reduced? i : 0;
        while(str[i]) 
            if(!search_key[i++-ofs]) return false;
        return true;
    }

    bool probe(char ch) 
    {
	char str[] = { ch, '\0' };
	size_t _ = 0;
	return link::find_node(str,_);
    }

    trie& insert(const char* str, const size_t ofs = 0)
    {
        if(search_key && match_tail(str,ofs))
	    return *this;
	else {
	    const char has_tail = search_key? search_key[Reduced?0:ofs] : 0;
	    #if DEMOTE == 4
	    bool conflict;
            if(has_tail && (!str[ofs] || 
		(conflict=probe(has_tail)) == probe(str[ofs]) && shorter_than_tail(str,ofs) || conflict)) {
	    #elif DEMOTE == 3
            if(has_tail && shorter_than_tail(str,ofs)) {
	    #elif DEMOTE == 1 || DEMOTE == 2
            if(has_tail && (has_tail == str[ofs] || str[ofs] == '\0')) {
	    #else
            if(has_tail) {
	    #endif
		link::select_node(search_key, Reduced?0:ofs).set(*this);
		if(*search_key) delete[] search_key;
		search_key = 0;
            }
	    #if DEMOTE > 2
	    if(!search_key) {
	    #elif DEMOTE == 2
	    if(str[ofs] == '\0' || !search_key && !probe(str[ofs])) {
	    #else
	    if(str[ofs] == '\0') {
	    #endif
		search_key = own_key(str,ofs);
		return *this;
            } else 
		return link::select_node(str, ofs);
        }
    }

    typename value<T>::reference operator[](const char* str) 
    { return insert(str).info; }
};


template<class T, template <class,class> class Link = CompactVector, class Key = char>
struct simple_trie : Link<simple_trie<T,Link,Key>, Key>, value<T> {
    typedef typename Link<simple_trie,Key>::link link;
    typedef typename link::pointer pointer;
    typedef Key key_type;
    typedef simple_trie trie_type;
    enum { full_key = false };

    simple_trie(bool is_key=false) 
    : link(), search_key(is_key) { }

    bool search_key;

    static simple_trie& create(simple_trie*& node, const char* key, size_t ofs=0)
    {
	node = new simple_trie(key[ofs] == '\0');
	if(key[ofs]) 
	    return node->link::select_node(key,ofs);
	else
	    return *node;
    }
      
    bool match_tail(const char* str, size_t i=0) const
    {
	return str[i] == '\0';
    }

    const simple_trie* search(const char* str)
    {
        simple_trie* cur_trie = this;
	size_t ofs = 0;
        do if(cur_trie->search_key && str[ofs] == '\0')
            return cur_trie;
        else
	    cur_trie = str[ofs]? cur_trie->find_node(str,ofs,OPTIMIZE) : 0;
        while(cur_trie);
	return 0;
    }

    simple_trie& insert(const char* str, const size_t ofs=0)
    {
	if(str[ofs] == '\0') {
	    search_key = true;
	    return *this;
	} else 
	    return link::select_node(str, ofs);
    }

    typename value<T>::reference operator[](const char* str) 
    { return insert(str).info; }
};


template<class T> 
size_t memused(T const& t, size_t allocated(size_t) = allocated)
{
    const typename T::link& link = t;
    return allocated(sizeof(T)) + memused(t.search_key, allocated) + memused(link, allocated);
}

template<> size_t memused(bool const& s,size_t allocated(size_t))
{
    return 0;
}

template<> size_t memused(char const& s,size_t allocated(size_t))
{
    return 0;
}

template<> size_t memused(const char* const& s,size_t allocated(size_t))
{
    return s && *s? allocated(std::strlen(s)+1) : 0;
}

template<size_t N> size_t memused(char_store<N> const& s,size_t allocated(size_t) = allocated)
{
    return 0;
}
/*
template<> size_t memused(char_store<7> const& s,size_t allocated(size_t))
{
    return 0;
}
template<> size_t memused(char_store<8> const& s,size_t allocated(size_t))
{
    return 0;
}
*/

template<> size_t memused(char_ptr const& s,size_t allocated(size_t))
{
    return memused<>(const_cast<const char*>(s.data), allocated);
}

template<class T, class K, template<class,class> class U>
size_t memused(const U<T,K>&, size_t allocated(size_t) = allocated)
{
    return 0;
}

template<class T, class K>
size_t memused(const SortedVector<T,K>& t, size_t allocated(size_t) = allocated)
{
    return allocated(t.capacity()*sizeof(typename Vector<T,K>::value_type));
}

template<class T, class K>
size_t memused(const Vector<T,K>& t, size_t allocated(size_t) = allocated)
{
    return allocated(t.capacity()*sizeof(typename Vector<T,K>::value_type));
}

template<class T, class K>
size_t memused(const CompactVector<T,K>& t, size_t allocated(size_t) = allocated)
{
    size_t capa = t.capacity();
    return allocated(capa+2+capa*sizeof(T*));
}

template<class T, class K>
size_t memused(const IndirectVector<T,K>& t, size_t allocated(size_t) = allocated)
{
    return allocated(t.capacity()*sizeof(T*));
}
