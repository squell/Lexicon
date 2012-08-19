#pragma once
#include <cstddef>

 // speed up the first lookup in a trie by using an array

template<class T>
struct turbo {
    typedef typename T::link link;
    typedef typename link::pointer pointer;
    typedef typename T::key_type key_type;
    typedef typename T::trie_type trie_type;

    T* const dict;
    T* lut[256];

    turbo(T* dict) : dict(dict), lut() { }

    turbo* operator->() { return this; }

    const T* search(const char* str)
    {
	assert(dict);
	key_type(' ');
        if(dict->search_key && dict->match_tail(str,0))
            return dict;
        else {
	    if(T* entry = lut[*str&0xFF]) 
		return entry->search(str+1);
	    else if(*str) {
		std::size_t ofs = 0;
		return (lut[*str&0xFF]=dict->find_node(str,ofs))->search(str+1);
	    } else
		return 0;
	}
    }

    T& insert(const char* str, const std::size_t ofs=0)
    {
	T& result = dict.insert(str, ofs);
	lut[*str & 0xFF] = 0;
	return result;
    }
};
