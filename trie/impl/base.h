#pragma once

#include <utility>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

template<class Key> struct key_traits;
template<> struct key_traits<char> {
    static size_t length(char)
    { return 1; }

    static bool match_key(char key, const char* test, size_t& ofs)
    { return ++ofs, true; }

    static char extract_key(const char* str, size_t& ofs)
    { return str[ofs++]; }

    template<class T>
    static T* split_key(T*& node, char& key, size_t split_pos, const char* str, size_t ofs)
    //static char split_key(char key, size_t i)
    { assert(!"key_traits<char>::split_key called."); }
};

struct char_ptr {
    char_ptr(char* data = 0) : data(data) { }
    operator char() const            { assert(data); return *data; }
    char& operator[](size_t i) const { assert(data); return data[i]; }
    char* data;
};

template<size_t N>
struct char_store {
    char_store() : data()            { }
    operator char() const            { return *data; }
    char& operator[](size_t i)       { return data[i]; }
    char data[N];
};

typedef char_store<sizeof(void*)> char_word;

template<> struct key_traits<char_ptr> {
    static size_t length(char_ptr key)
    { return std::strlen(key.data); }

    static bool match_key(char_ptr key, const char* test, size_t& ofs)
    { 
	size_t const base = ofs;
	do { 
	    ++ofs;
	    if(!key[ofs-base]) return true;
	} while(test[ofs] == key[ofs-base]);
	return false;
    }

    static char_ptr extract_key(const char* str, size_t& ofs)
    { 
	str += ofs;
	size_t const len = std::strlen(str);
	ofs += len;
	return *str? std::strcpy(new char[len+1], str) : const_cast<char*>("");
    }

    // not exception safe
    template<class T>
    static T* split_key(T*& node, char_ptr& key, size_t split_pos, const char* str, size_t ofs)
    { 
	char_ptr subkey = std::strcpy(new char[std::strlen(&key[split_pos])+1], &key[split_pos]);
        char_ptr newkey = extract_key(str, ofs);
	key[split_pos] = '\0';
        T* subnode = node;
        T* newnode = 0;
	T* endnode = &T::create(newnode,str,ofs);
	if(newkey[0]) 
	    (node=new T)->attach_node(newkey, newnode);
	else 
	    node = newnode;
	node->attach_node(subkey, subnode);
	return endnode;
    }
};

template<size_t N> struct key_traits< char_store<N> > {
private:
    typedef char_store<N> char_word;
public:
    static size_t length(char_word key)
    { size_t i = 0; while(i < sizeof key.data && key.data[i]) ++i; return i; }

    static bool match_key(char_word key, const char* test, size_t& ofs)
    { 
	++ofs;
	for(unsigned i=1; i < sizeof key.data && key[i]; ++i, ++ofs) {
	    if(test[ofs] != key[i]) return false;
	}
	return true;
    }

    static char_word extract_key(const char* str, size_t& ofs)
    { 
	char_word tmp;
	for(unsigned i=0; i < sizeof tmp.data; ++i, ++ofs)
	    if(tmp[i] = str[ofs]); else break;
	return tmp;
    }

    // not exception safe
    template<class T>
    static T* split_key(T*& node, char_word& key, const size_t split_pos, const char* str, size_t ofs)
    { 
	char_word subkey;
        char_word newkey = extract_key(str, ofs);
	for(unsigned i=split_pos; i < sizeof subkey.data; ++i)
	    if(subkey.data[i-split_pos] = key.data[i]); else break;
	key[split_pos] = '\0';
        T* subnode = node;
        T* newnode = 0;
	T* endnode = &T::create(newnode,str,ofs);
	if(newkey[0]) 
	    (node=new T)->attach_node(newkey, newnode);
	else 
	    node = newnode;
	node->attach_node(subkey, subnode);
	return endnode;
    }
};
