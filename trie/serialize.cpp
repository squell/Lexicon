#pragma once

#include <cstddef>
#include <cstring>
#include <istream>
#include <ostream>
#include <streambuf>
#include "basis.cpp"
#include "impl/base.h"

// TODO: safety in read (unchecked: can wander off...)

class serialize {

    enum cookie { var_len };

    serialize(std::streambuf* sb) 
    : char_count(), node_count(), sb(sb) { }

    void out(size_t value, int size);
    void out(size_t value, cookie);

    void out(bool b);
    void out(const char* str);
    void out(char c);
    void out(char_ptr key);
    template<size_t N>
      void out(char_store<N> key);

    template<class T>
    void out(const value<T>& data);

    template<class T>
    void out(T& lex);

    size_t char_count, node_count;
    std::streambuf* const sb;

public:
    template<class T>
    void operator()(typename T::key_type key, T& lex);

    template<class T>
    static void write(std::ostream&, T*);
};

template<class T>
void serialize::write(std::ostream& out, T* lexicon)
{
    serialize writer(out.rdbuf());
    writer.out(*static_cast<T*>(lexicon));
    writer.out(writer.node_count, 8);
    writer.out(writer.char_count, 8);
}

void serialize::out(size_t value, int size)
{
    char data[sizeof(value)];
    for(int i=0; i < size; ++i)
	data[i] = (value >> i*8) & 0xFF;
    sb->sputn(data, size);
}

void serialize::out(size_t value, cookie)
{
#if FIXED_WIDTH
    out(value, 2);
#else
    while(value & ~0x7F) {
	sb->sputc(value & 0x7F | 0x80);
	value >>= 7;
    }
    sb->sputc(value);
#endif
}

void serialize::out(char c)
{
    sb->sputc(c);
}

void serialize::out(bool b)
{
    sb->sputc(b);
}

void serialize::out(const char* str)
{
    if(!str) return out(0, var_len);
    size_t len = std::strlen(str);
    out(len+1,var_len);
    sb->sputn(str, len);
#if SINGLE_STR
    if(len > 1) char_count += len+1;
#else
    if(len) char_count += len+1;
#endif
}

void serialize::out(char_ptr key)
{
    const char* str = key.data;
    out(str);
}

template<size_t N>
void serialize::out(char_store<N> key)
{
    for(int i=0; i < sizeof(key.data); ++i)
	out(key.data[i]);
}

template<class T>
void serialize::out(const value<T>& data)
{
    size_t const N = sizeof(T);
    unsigned char buf[N];
    std::memcpy(buf, &data.info, N);
    sb->sputn(buf, N);
}

template<>
void serialize::out(const value<void>&) { }

template<class T>
void serialize::operator()(typename T::key_type key, T& lex)
{
    out(key);
    out(lex);
}

template<class T>
void serialize::out(T& lex)
{
    out(lex.search_key);
    node_count++;
    out(lex.arity(),var_len);
    lex.template explore<serialize&>(*this);
}

class unserialize {
    std::streambuf* const sb;
protected:
    enum cookie { var_len };

    unserialize(std::streambuf* sb) : sb(sb) { }

    bool in(size_t& value, int size);
    bool in(size_t& value, cookie);
    bool in(bool& c);
    bool in(const char*& c);
    bool in(char& c);
    bool in(char_ptr& c);
    template<size_t N>
    bool in(char_store<N>& c);

    template<class T>
    bool in(value<T>& data);

    char* text;

public:
    template<class T>
    static std::istream& read(std::istream& in, T*& lex);
};

template<class T>
class unserialize_t : unserialize {
    friend class unserialize;
    T* node;

    using unserialize::in;
    bool in();

    unserialize_t(std::streambuf* sb) 
    : unserialize(sb) { }
};

template<class T>
std::istream& unserialize::read(std::istream& in, T*& lex)
{
    char* text_buf = 0;
    T* node_buf = 0;
    try {
	std::streambuf* sb = in.rdbuf();
	unserialize_t<T> reader(sb);
	sb->pubseekoff(-16,std::ios::end);
	size_t nodes, bytes;
	bool ok = reader.in(nodes, 8) && reader.in(bytes, 8);
	sb->pubseekoff(0,std::ios::beg);
	if(!ok) 
	    return in.setstate(std::istream::failbit), in;
	reader.text = text_buf = new char[bytes];
	reader.node = node_buf = new T[nodes];
	//printf(">> %ld\n", (bytes + sizeof(T)*nodes) / 1024);
	if(reader.in()) 
	    return lex = node_buf, in;
	in.setstate(std::istream::failbit);
    } catch(...) {
	in.setstate(std::istream::badbit);
    }
    delete[] text_buf;
    delete[] node_buf;
    return in;
}

bool unserialize::in(size_t& value, int size)
{
    char data[sizeof(value)];
    if(sb->sgetn(data, size) != size) 
	return false;
    value = 0;
    for(int i=0; i < size; ++i)
	value |= (data[i]&0xFF) << i*8;
    //printf("[] Suc6 rd: 0x%x\n", int(value));
    return true;
}

bool unserialize::in(size_t& value, cookie)
{
#if FIXED_WIDTH
    return in(value, 2);
#else
    size_t acc = 0;
    int c, len = 0;
    do {
	c = sb->sbumpc();
	if(c == std::streambuf::traits_type::eof())
	    return false;
	acc |= (c & 0x7F) << len;
	len += 7;
    } while(c & ~0x7F);
    return value = acc, true;
#endif
}

bool unserialize::in(char& c)
{
    int x = sb->sbumpc();
    if(x != std::streambuf::traits_type::eof()) 
	return c=x, true;
    else
	return false;
}

bool unserialize::in(bool& b)
{
    char c;
    if(in(c)) return b = c, true;
    else return false;
}

bool unserialize::in(const char*& str)
{
    size_t len;
    if(!in(len,var_len))
	return false;
    if(len-- == 0)
	return str = 0, true;
    if(len == 0)
	return str = "", true;
    if(sb->sgetn(text, len) != len) 
	return false;
#if SINGLE_STR
    static char tab[256][2];
    if(!tab[255][0]) 
	for(int i=0; i < 256; ++i) tab[i][0] = i;
    if(len == 1) 
	return str = tab[*text&0xFF], true;
#endif
    str = text;
    text += len;
    *text++ = 0;
    //printf("[] Suc6 rd: '%s'\n", str);
    return true;
}

bool unserialize::in(char_ptr& key)
{
    return in(const_cast<const char*&>(key.data));
}

template<size_t N>
bool unserialize::in(char_store<N>& key)
{
    bool ok = true;
    for(int i=0; i < sizeof(key.data); ++i)
	ok &= in(key.data[i]);
    return ok;
}

template<class T>
bool unserialize::in(value<T>& data)
{
    size_t const N = sizeof(T);
    unsigned char buf[N];
    if(sb->sgetn(buf, N) != N)
	return false;
    std::memcpy(&data.info, buf, N);
    return true;
}

template<>
bool unserialize::in(value<void>&) { return true; }

template<class T>
bool unserialize_t<T>::in()
{
    size_t arity;
    T& lex = *node++; 
    if(!in(lex.search_key) || !in(arity,var_len))
	return false;
    lex.reserve(arity);
    for(int i=0; i < arity; ++i) {
	typename T::key_type key;
	T* const cur = node;
	if(in(key) && in())
	    lex.attach_node(key,cur);
	else
	    return false;
    }
    //printf("[X] %s [%d]\n", lex.search_key, (int)lex.arity());
    return true;
}
