#pragma once
#include <algorithm>
#include <utility>
#include <functional>
#include <cstring>

// predicat to be used for sorting structures on a single field

template<class T, class C, class pred>
class onkey_t : pred {
    T C::* const key;
public:
    bool operator()(const C& a, const C& b) const
    {
	return pred::operator()(a.*key, b.*key);
    }
    onkey_t(T C::* key, pred p) : key(key), pred(p) { }
};

template<class T, class C, class pred>
inline onkey_t<T,C,pred> onkey(T C::* key, pred p)
{
    return onkey_t<T,C,pred>(key,p);
}

template<class T, class C>
inline onkey_t<T,C,std::less<T> > onkey(T C::* key)
{
    return onkey(key, std::less<T>());
}

/* A memory-compact alternative to map<char,*T>
 * - requires keys to be unique (not checked)
*/
template<class T>
struct compact_association_vector {
    typedef std::pair<char, T*> value_type;

    compact_association_vector() : tails(), keys(const_cast<char*>("")) { }

    ~compact_association_vector() 
    {
	if(tails) {
	    while(keys[-1] == 0) 
		keys--, tails--;
  	    delete[] tails;
	}
    }

    bool empty() const
    {
	return !*keys;
    }

    size_t size() const 
    {
	return std::strlen(keys);
    }

    void reserve(size_t n)
    { 
	reserve(n, size());
    }

    void reserve(size_t n, size_t len)
    {
	if(len > n) return;

	unsigned char* buffer = new unsigned char[n*(sizeof(T*)+1)+2];
	T** ntails  = (T**)   buffer;
	char* nkeys = (char*) (buffer+n*sizeof(T*));
	*nkeys++ = 1;
	std::memset(nkeys,  0, n-len);
	nkeys  += n-len;
	ntails += n-len;
	std::memcpy(nkeys,  keys,  len+1);
	std::memcpy(ntails, tails, len*sizeof(T*));
	if(tails) {
	    while(keys[-1] == 0) 
		keys--, tails--;
	    delete[] tails;
	}
	keys = nkeys, tails = ntails;
    }

    void push_back(const value_type& pair)
    {
	if(!tails || keys[-1]) {
	    size_t len = size();
	    reserve(len+2, len);
	    //reserve((len+8)&~0x7, len);
	}
	*--keys  = pair.first;
	*--tails = pair.second;
    }

    size_t capacity() const
    {
        char* p = keys;
	while(p[-1] == '\0') --p;
	return size() + (keys-p);
    }

    // doesn't implement the full iterator requirements, just what we need
    // still a lot of plumbing :(
    struct proxy : value_type {
	proxy(const value_type& v) : value_type(v) { }
	value_type* operator->()    { return this; }
    };

    struct iterator 
    {
	iterator() : p(""), cont(0) { }
	iterator(compact_association_vector& cont) : cont(&cont), p(cont.keys) { }
	compact_association_vector* const cont;
	const char* p;
	operator value_type() const 
	{ return value_type(*p, cont->tails[p - cont->keys]); }
	iterator& operator=(const iterator& other)
	{ return p = other.p, *this; }
	void operator=(const value_type& val) const
	{ size_t const i = p - cont->keys;
	  cont->keys [i] = val.first;
	  cont->tails[i] = val.second; }
	iterator& operator++() 
	{ return ++p, *this; }
	iterator& operator--() 
	{ return --p, *this; }
	const iterator& operator*() const
	{ return *this; }
	proxy operator->() const
	{ return proxy(*this); }
	bool operator!=(const iterator& other) const
	{ return *p != *other.p; }
	bool operator==(const iterator& other) const
	{ return *p == *other.p; }
    };

    iterator begin() { return iterator(*this); }
    iterator end()   { return iterator(); }

    iterator operator[](size_t pos) 
    {
	iterator tmp(*this);
	tmp.p += pos;
	return tmp;
    }

    void iter_swap(const iterator& p, const iterator& q)
    {
	size_t const i = p.p-keys;
	size_t const j = q.p-keys;
	std::swap(keys[i], keys[j]);
	std::swap(tails[i], tails[j]);
    }

protected:
    char* keys;
    T**   tails;
private:
    compact_association_vector(const compact_association_vector&);
};

