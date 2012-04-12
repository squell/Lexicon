#include <utility>
#include <map>
#include <tr1/unordered_map>
#include <tr1/memory>
#include <tr1/tuple>
#include <stdio.h>
#include <string.h>

/* A easy (but not necessarily cheap) way of performing memoization */

// FIXME: lut should not be in a sharedptr?

template<class F, class R, class A>
class memo_t {
    typedef std::tr1::unordered_map<A,R> hash_table;
    //typedef std::map<A,R> hash_table;
    const std::tr1::shared_ptr<hash_table> lut;
    const F fun;
public:
    memo_t(F f) : fun(f), lut(new hash_table) { }

    template<class P>
    R operator()(P key)
    {
	typename hash_table::iterator p = lut->find(key);
	if(p != lut->end()) return p->second;
	R tmp(fun(key));
	lut->insert(std::make_pair(key,tmp));
	return tmp;
    }

    template<class P1, class P2>
    R operator()(P1 arg1, P2 arg2)
    {
	std::pair<P1,P2> key = std::make_pair(arg1,arg2);
	typename hash_table::iterator p = lut->find(key);
	if(p != lut->end()) return p->second;
	R tmp(fun(arg1,arg2));
	lut->insert(p, std::make_pair(key,tmp));
	return tmp;
    }

    template<class P1, class P2, class P3>
    R operator()(P1 arg1, P2 arg2, P3 arg3)
    {
	std::tr1::tuple<P1,P2,P3> key = std::tr1::make_tuple(arg1,arg2,arg3);
	typename hash_table::iterator p = lut->find(key);
	if(p != lut->end()) return p->second;
	R tmp(fun(arg1,arg2,arg3));
	lut->insert(p, std::make_pair(key,tmp));
	return tmp;
    }
};

// instantiator functions for function pointers

template<class A,class R>
memo_t<R (*)(A),R,A> memo(R (*fptr)(A))
{
    return memo_t<R(*)(A),R,A>(fptr);
}

template<class A1,class A2,class R>
memo_t<R (*)(A1,A2),R,std::pair<A1,A2> > memo(R (*fptr)(A1,A2))
{
    return memo_t<R(*)(A1,A2),R,std::pair<A1,A2> >(fptr);
}

template<class A1,class A2,class A3,class R>
memo_t<R (*)(A1,A2,A3),R,std::tr1::tuple<A1,A2,A3> > memo(R (*fptr)(A1,A2,A3))
{
    return memo_t<R(*)(A1,A2,A3),R,std::tr1::tuple<A1,A2,A3> >(fptr);
}

// specialize hash function

namespace std {
namespace tr1 {
    template<> 
    struct hash<std::pair<const char*,const char*> > {
	std::size_t operator()(std::pair<const char*,const char*> obj) const 
	{
	    std::tr1::hash<const char*> h;
	    return h(obj.first) ^ h(obj.second);
	}
    };
}
}
