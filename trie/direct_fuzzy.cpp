#pragma once

 /* Perform edit-distance computation directly on the trie using 
    straight-forward recursion + memoisation. */

#include <utility>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <tr1/unordered_set>
#include <fstream>

#include "impl/base.h"

#define BEST_FIRST 2
//#define HASH

#ifdef HASH
struct pairhash {
    size_t operator()(std::pair<const void*,const void*> obj) const
    {
        std::tr1::hash<const void*> h;
        return h(obj.first) ^ h(obj.second);
    }
};
#endif

struct penalty_default {
    static unsigned del(char c) { return 1; }
    static unsigned ins(char) { return 1; }
    static unsigned rpl(char a, char b) { return a != b; }
};

struct penalty_class {
    static bool is_vowel(char c)
    {
	return strchr("eahiou", c);
    }
    static bool is_s(char c)
    {
	return strchr("csz", c);
    }
    static bool is_m(char c)
    {
	return strchr("mn", c);
    }
    static bool is_b(char c)
    {
	return strchr("bpvw", c);
    }
    static bool is_f(char c)
    {
	return strchr("vwf", c);
    }
    static bool is_k(char c)
    {
	return strchr("ckgh", c);
    }
    static bool is_t(char c)
    {
	return strchr("dt", c);
    }
    static unsigned del(char c) { return is_k(c)||is_t(c)? 1 : is_m(c)? 2 : is_s(c)||is_vowel(c)? 3 : 4; }
    static unsigned ins(char c) { return is_k(c)||is_vowel(c)? 1 : is_t(c)? 2 : 4; }
    static unsigned rpl(char a, char b) 
    { 
	if(toupper(a)==toupper(b)) return 0;
	if(is_vowel(a) && is_vowel(b)) return 1;
	if(is_s(a) && is_s(b)) return 1;
	if(is_m(a) && is_m(b)) return 1;
	if(is_b(a) && is_b(b)) return 1;
	if(is_f(a) && is_f(b)) return 2;
	if(is_k(a) && is_k(b)) return 2;
	if(is_t(a) && is_t(b)) return 2;
	if(is_s(a) && is_t(b) || is_t(a) && is_s(b)) return 3;
	if(is_b(a) && is_f(b) || is_f(a) && is_b(b)) return 3;
	return 5;
    }
};

struct penalty_table {
    unsigned char matrix[256][256];
    unsigned del(char c) const { return matrix[c&0xFF][0]; }
    unsigned ins(char c) const { return matrix[0][c&0xFF]; }
    unsigned rpl(char a, char b) const { return matrix[a&0xFF][b&0xFF]; }
};

struct penalty_random : penalty_table {
    penalty_random() 
    {
	for(int i=0; i < 256; i++)
	    for(int j=0; j < 256; j++)
		matrix[i][j] = i==j? 0 : rand()%8 + 1;
    }
};

struct penalty_file : penalty_table 
{
    penalty_file(const char* filename)
    {
	std::ifstream stored(filename);
	for(int i=0; i < 256; i++)
	    for(int j=0; j < 256; j++) {
		size_t d;
		stored >> d;
		matrix[i][j] = d;
	    }
	if(!stored.good()) 
	    throw "Well, that was to be expected...";
    }
};

template<class Trie, class Penalties = penalty_default> 
struct direct_fuzzy : Trie {
    typedef typename Trie::link link;
    typedef std::pair<const direct_fuzzy*,unsigned> result;

    static unsigned match(const Penalties& cost, bool, const char* str, unsigned limit=-1)
    { 
	unsigned dist = 0;
	while(char c = *str++)
	    if((dist += cost.del(c)) > limit) break;
	return dist;
    }

    static unsigned min3(unsigned a, unsigned b, unsigned c)
    {
	return std::min(std::min(a,b), c);
    }

    static unsigned match(const Penalties& cost, const char* pattern, const char* str, unsigned limit=-1)
    { 
	size_t pat_size = std::strlen(pattern);
	#if 0
	std::vector<unsigned> tab(pat_size+1);
	#else
	static unsigned tab[4096];
	#endif
	// initialize dynamic programming matrix
	unsigned dist = 0;
	for(size_t i=0; i <= pat_size; ++i) {
	    tab[i] = dist;
	    dist += cost.ins(pattern[i]);
	}

	// run through the remainder of the string
	while(char c = *str++) {
	    unsigned old_left = tab[0];
	    unsigned new_left = tab[0] += cost.del(c);
	    for(size_t i=1; i <= pat_size; ++i) {
		new_left = min3(
		    old_left + cost.rpl(c, pattern[i-1]),
		    tab[i]   + cost.del(c),
		    new_left + cost.ins(pattern[i-1])
		  );
		old_left = tab[i];
		tab[i] = new_left;
	    }
	}
	return tab[pat_size];
    }

    unsigned match_tail(const Penalties& cost, const char* str, size_t ofs=0)
    {
	if(Trie::full_key)
	    return match(cost, search_key+ofs, str);
	else
	    return match(cost, search_key, str);
    }

    /*
    static void prune(std::vector<result>& results, unsigned limit)
    {
	typename std::vector<result>::iterator p, q;
	for(p = q = results.begin(); p != results.end(); ++p) 
	    if(p->second <= limit) *q++ = *p;
	results.erase(q,results.end());
    }
    */

    using Trie::search;
    const Trie* search(const char* str, unsigned limit)
    {
	std::vector<result> res;
	search_fuzzy(res, str, limit, 2);
	if(res.empty())
	    return 0;
	else
	    return res[0].first;
    }

    std::vector<result> search_fuzzy(const char* str, unsigned limit, char mode=true, const Penalties& dist_table = Penalties())
    {
	std::vector<result> res;
	search_fuzzy(res, str, limit, mode, dist_table);
	return res;
    }

    typedef std::pair<const Trie*, const char*> memo_key;
#ifdef HASH
    typedef std::tr1::unordered_set<memo_key, pairhash> memo_table;
#else
    typedef std::set<memo_key> memo_table;
#endif

    void search_fuzzy(std::vector<result>& res, const char* str, unsigned limit, char mode=true, const Penalties& dist_table = Penalties())
    {
	memo_table memo;
	search_recursive(memo, res, str, 0, 0, limit, mode, dist_table);
    }

    struct feeder {
	const Penalties& cost;
#if BEST_FIRST
	struct held { direct_fuzzy* trie; const char* str; size_t ofs; };
	std::vector<held>* const delayed;
#endif
	memo_table& memo;
	std::vector<result>& results;
	const char* const str;
	size_t const ofs;
	unsigned const dist;
	unsigned& limit;
	char const best_only;

	void operator()(char c, Trie& entry) const
	{ 
//printf("{open:%*c:%d}\n", ofs*4, ' ', entry.search_key);
	    direct_fuzzy* trie = static_cast<direct_fuzzy*>(&entry);
	    const char* inp = str;
	    unsigned ndist = dist;
	    unsigned penalty;
	    while(char ct = *inp++) {
		if(ndist+(penalty=cost.rpl(ct, c)) <= limit) 
//printf("%*c", ofs*4, ' '), printf("[%c==%c]\n", c, ct),
		    enqueue(trie, inp, ofs+1, ndist+penalty);
		if(ndist+(penalty=cost.ins(c)) <= limit)
//printf("%*c", ofs*4, ' '), printf("[ins %c]\n", c),
		    enqueue(trie, inp-1, ofs+1, ndist+penalty);
		ndist += cost.del(ct);
		if(ndist > limit)
		    return;
//printf("%*c", ofs*4, ' '), printf("[del %c]\n", ct);
	    } 
	    if(ndist+(penalty=cost.ins(c)) <= limit) 
		enqueue(trie, inp-1, ofs+1, ndist+penalty);
	}

	void enqueue(direct_fuzzy* entry, const char* str, size_t ofs, unsigned ndist) const
	{
#if BEST_FIRST
	    if(ndist == dist) {
		if(!memo.insert(memo_key(entry, str)).second) 
		    return;
		feeder next = { cost, delayed, memo, results, str, ofs, ndist, limit, best_only };
		next.visit(entry);
		entry->template explore<feeder&>(next, 0);
	    } else if (ndist <= limit) {
		held record = { entry, str, ofs };
		delayed[ndist].push_back(record);
	    }
#else
	    entry->search_recursive(memo, results, str, ofs, ndist, limit, best_only, cost);
#endif
	}

	void visit(direct_fuzzy* entry) 
	{
            if(!entry->search_key) return;

            unsigned const ndist = dist + entry->match_tail(cost, str, ofs);

            if(ndist <= limit) {
                result record(entry,ndist);
                if(best_only && ndist < limit)
#if BEST_FIRST == 2
		    if((limit=ndist) == dist) {
			memo.insert(memo_key(entry, 0));
	    //printf("A:%d:%s\n", ndist, entry->search_key);
			results.assign(1,record);
		    } else {
	    //printf("a:%d:%s\n", ndist, entry->search_key);
			held record = { entry, 0, ofs };
			delayed[ndist].push_back(record);
		    }
#else
                    results.assign(1,record), limit = ndist;
#endif
                else if(best_only > 1 && !results.empty() && limit) 
	  	// && limit => what to do if multiple hits at distance 0?
		//printf("x:%d:%s\n", ndist, entry->search_key),
                    results.clear(), limit--;
                else
#if BEST_FIRST == 2
		    if(ndist == dist) {
	    //printf("B:%d:%s\n", ndist, entry->search_key);
			if(memo.insert(memo_key(entry, 0)).second) // is this really wise? (pollutes memo)
			    results.push_back(record);
		    } else {
	    //printf("b:%d:%s\n", ndist, entry->search_key);
			held record = { entry, 0, ofs };
			delayed[ndist].push_back(record);
		    }
#else
                    results.push_back(record);
#endif
            }
	}

    private:
	~feeder() {} 
	friend class direct_fuzzy;
    };

    void search_recursive(memo_table& memo, std::vector<result>& res, const char* str, size_t ofs, unsigned threshold, unsigned& limit, char best_only, const Penalties& dist_table)
    {
	std::pair<typename memo_table::iterator, bool> lookup = memo.insert(memo_key(this, str));
	if(!lookup.second) return;

#if BEST_FIRST
	typedef std::vector<typename feeder::held> bucket_vector;
	std::vector<bucket_vector> bucket_vec(1+limit); 
	bucket_vector* const bucket = bucket_vec.data();
	feeder recurse = { dist_table, bucket, memo, res, str, ofs, threshold, limit, best_only };
#else
	feeder recurse = { dist_table, memo, res, str, ofs, threshold, limit, best_only };
#endif
	recurse.visit(this);

	Trie::template explore<feeder&>(recurse, 0);

#if BEST_FIRST
	while(++threshold <= limit)
	    for(size_t i=0; i < bucket[threshold].size(); ++i) {
		typename feeder::held& r = bucket[threshold][i];
		if(!memo.insert(memo_key(r.trie, r.str)).second) 
		    continue;
#if BEST_FIRST == 2
		if(r.str == 0) {
	   //printf("D:%d:%s\n", threshold, r.trie->search_key);
		    if(best_only > 1 && res.size()) { // late addition
			res.clear();
			break;
		    } else
			res.push_back(result(r.trie, threshold));
		    continue;
		}
#endif
		feeder next = { dist_table, bucket, memo, res, r.str, r.ofs, threshold, limit, best_only };
		next.visit(r.trie);
		if(threshold > limit)  // late addition
		    break;
		r.trie->template explore<feeder&>(next, 0);
	    }
#endif
    }

    using Trie::search_key;
};

