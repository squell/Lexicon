#pragma once

 /* Perform edit-distance computation directly on the trie using
    straight-forward recursion + memoisation.

    Variation: instead of a 1->1 weight table, costs of edit ops are also
    influenced by a character of context immediately preceeding it. This
    turns out to be cheaply implemented. 

    Further variation: context comes from the word stored in the Lexicon,
    instead of the user input.
 */

#include <utility>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <tr1/unordered_set>
#include <fstream>

#include "impl/base.h"

#define BEST_FIRST 2

#define FTRANS 1
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
    static unsigned del(char, char) { return 1; }
    static unsigned ins(char,char) { return 1; }
    static unsigned rpl(char a, char b) { return a != b; }
    static unsigned swp(char,char) { return 1; }
};

struct penalty_table {
    unsigned char matrix[4][256][256];
    unsigned del(char c, char ctx) const { return matrix[2][c&0xFF][ctx&0xFF]; }
    unsigned ins(char c, char ctx) const { return matrix[1][c&0xFF][ctx&0xFF]; }
    unsigned rpl(char a, char b) const   { return matrix[0][a&0xFF][b&0xFF]; }
    unsigned swp(char a, char b) const   { return matrix[3][a&0xFF][b&0xFF]; }
};

struct penalty_file : penalty_table 
{
    penalty_file(const char* filename)
    {
	std::ifstream stored(filename);
	for(int t=0; t < 4; t++) 
	    for(int i=0; i < 256; i++)
		for(int j=0; j < 256; j++) {
		    size_t d;
		    stored >> d;
		    matrix[t][i][j] = d;
		}
	if(!stored.good()) 
	    throw "Well, that was to be expected...";
    }
};

template<class Trie, class Penalties = penalty_default> 
struct direct_fuzzy : Trie {
    typedef typename Trie::link link;
    typedef std::pair<const direct_fuzzy*,unsigned> result;

    static unsigned match(const Penalties& cost, bool, const char* str, char prev_c=0, unsigned limit=-1)
    { 
	unsigned dist = 0;
	while(char c = *str++) {
	    if((dist += cost.del(c,prev_c)) > limit) break;
	    prev_c = c;
	}
	return dist;
    }

    static unsigned min3(unsigned a, unsigned b, unsigned c)
    {
	return std::min(std::min(a,b), c);
    }

    static unsigned match(const Penalties& cost, const char* pattern, const char* str, char prev_c=0, unsigned limit=-1)
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
	    dist += cost.ins(pattern[i], 0);
	}

	// run through the remainder of the string
	while(char c = *str++) {
	    unsigned old_left = tab[0];
	    unsigned new_left = tab[0] += cost.del(c, prev_c);
	    for(size_t i=1; i <= pat_size; ++i) {
		new_left = min3(
		    old_left + cost.rpl(pattern[i-1], c),
		    tab[i]   + cost.del(c, prev_c),
		    new_left + cost.ins(pattern[i-1], prev_c)
		  );
		old_left = tab[i];
		tab[i] = new_left;
		prev_c = c;
	    }
	}
	return tab[pat_size];
    }

    unsigned match_tail(const Penalties& cost, const char* str, char context, size_t ofs=0)
    {
	if(Trie::full_key)
	    return match(cost, search_key+ofs, str,context);
	else
	    return match(cost, search_key, str,context);
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
	struct held { direct_fuzzy* trie; const char* str; size_t ofs; char context; };
	std::vector<held>* const delayed;
#endif
	memo_table& memo;
	std::vector<result>& results;
	const char* const str;
	size_t const ofs;
	unsigned const dist;
	unsigned& limit;
	char const best_only;
	char last_ct_store;

	void operator()(char c, Trie& entry) const
	{ 
//printf("{open:%*c:%d}\n", ofs*4, ' ', entry.search_key);
	    direct_fuzzy* trie = static_cast<direct_fuzzy*>(&entry);
	    const char* inp = str;
	    unsigned ndist = dist;
	    unsigned penalty;
	    char last_ct = last_ct_store;
	    while(char ct = *inp++) {
		if(ndist+(penalty=cost.rpl(c, ct)) <= limit) 
//printf("%*c", ofs*4, ' '), printf("[%c==%c]\n", c, ct),
		    enqueue(trie, inp, ofs+1, ndist+penalty, ct);
		if(ndist+(penalty=cost.ins(c,last_ct)) <= limit)
//printf("%*c", ofs*4, ' '), printf("[ins %c]\n", c),
		    enqueue(trie, inp-1, ofs+1, ndist+penalty, c);
#if FTRANS
                if(*inp == c && ndist+(penalty=cost.swp(ct,c)) <= limit) {
                    char tmp[2] = { ct, 0 };
                    size_t zero = 0;
//printf("%*c", ofs*4, ' '), printf("[%c<->%c]\n",  ct, c),
                    if(Trie* p = entry.find_node(tmp, zero, 0))
                        enqueue(static_cast<direct_fuzzy*>(p), inp+1, ofs+2, ndist+penalty, c);
                }
#endif
		ndist += cost.del(ct, last_ct);
		last_ct = ct;
		if(ndist > limit)
		    return;
//printf("%*c", ofs*4, ' '), printf("[del %c]\n", ct);
	    } 
	    if(ndist+(penalty=cost.ins(c,last_ct)) <= limit) 
		enqueue(trie, inp-1, ofs+1, ndist+penalty, last_ct);
	}

	void enqueue(direct_fuzzy* entry, const char* str, size_t ofs, unsigned ndist, char context) const
	{
#if BEST_FIRST
	    if(ndist == dist) {
		if(!memo.insert(memo_key(entry, str)).second) 
		    return;
		feeder next = { cost, delayed, memo, results, str, ofs, ndist, limit, best_only, context };
		next.visit(entry);
		entry->template explore<feeder&>(next, 0);
	    } else if (ndist <= limit) {
		held record = { entry, str, ofs, context };
		delayed[ndist].push_back(record);
	    }
#else
	    entry->search_recursive(memo, results, str, ofs, ndist, limit, best_only, cost);
#endif
	}

	void visit(direct_fuzzy* entry) 
	{
            if(!entry->search_key) return;

            unsigned const ndist = dist + entry->match_tail(cost, str, last_ct_store, ofs);

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
		feeder next = { dist_table, bucket, memo, res, r.str, r.ofs, threshold, limit, best_only, r.context };
		next.visit(r.trie);
		if(threshold > limit)  // late addition
		    break;
		r.trie->template explore<feeder&>(next, 0);
	    }
#endif
    }

    using Trie::search_key;
};

