#pragma once
#warning "NOT FINISHED"

 /* Attempt at grafting a generalized (n->n, with weights) edit distance
    onto a fuzzy trie search */

#include <utility>
#include <algorithm>
#include <vector>
#include <map>
#include <set>

#include "impl/base.h"
#include "basis.cpp"

#undef BEST_FIRST
#define BEST_FIRST 2

#define HARDCODED 0 

typedef std::pair<const char*,unsigned int> cost;

typedef simple_trie< std::vector<cost> > penalty_trie;

struct symmetric_penalty_trie : penalty_trie {
    void add(const char* a, const char* b, unsigned cost)
    {
	(*this)[a].push_back(std::make_pair(b,cost));
	(*this)[b].push_back(std::make_pair(a,cost));
    }

static bool optimize_order(char, penalty_trie& trie, int)
{
    sort(trie.info.begin(), trie.info.end(), onkey(&cost::second));
    return true;
}

symmetric_penalty_trie()
{
    char sing[256][2];
    for(char c='a'; c <= 'z'; c++) {
	sing[c][1] = '\0';
	sing[c][0] = c;
	add(sing[c], sing[c], 0);
    }
    add(" ", "", 4);

    add("i", "y", 1);
    add("xx", "cks", 1);
    add("ice", "ise", 1);
    add("yze", "yse", 0);
    add("our", "or",  0);
    add("ou", "oe",  1);
    add("ough", "u",  1);
    add("eye", "aye",  1);
    add("th", "f",  2);
    walk(optimize_order);
}
} confusion;

// sort a node on weights: best transitions first

//////////////////////////////////////////////

template<class Trie, class Penalty = penalty_trie> 
struct general_fuzzy : Trie {
    typedef typename Trie::link link;
    typedef std::pair<const general_fuzzy*,unsigned> result;

    static const char* prefixes(const char* a, const char* b)
    {
	do {
	    if(*a == '\0') return b;
#if HARDCODED
	} while(*a++ == *b++ || a[-1] == '\a');
#else
	} while(*a++ == *b++);
#endif
	return 0;
    }

    static unsigned match(bool, const char* str, unsigned limit=-1)
    { 
	/* TODO
	unsigned dist = 0;
	while(char c = *str++)
	    if((dist += cost.del(c)) > limit) break;
	return dist;
	*/
	return *str? 10000 : 0;
    }

    static unsigned match(const char* pattern, const char* str, unsigned limit=-1)
    { 
	/* TODO
	 * */
    }

    unsigned match_tail(const char* str, size_t ofs=0)
    {
	if(Trie::full_key)
	    return match(search_key+ofs, str);
	else
	    return match(search_key, str);
    }

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

    std::vector<result> search_fuzzy(const char* str, unsigned limit, char mode=true)
    {
	std::vector<result> res;
	search_fuzzy(res, str, limit, mode);
	return res;
    }

    typedef std::pair<const Trie*, const char*> memo_key;
    typedef std::set<memo_key> memo_table;

    void search_fuzzy(std::vector<result>& res, const char* str, unsigned limit, char mode=true)
    {
	memo_table memo;
	search_recursive(memo, res, str, 0, 0, limit, mode);
    }

    struct feeder {
	Penalty* const cur_cost;
	struct held { general_fuzzy* trie; Penalty* cost; const char* str; size_t ofs; };
	std::vector<held>* const delayed;
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
	    general_fuzzy* trie = static_cast<general_fuzzy*>(&entry);
#if HARDCODED
	    if(cur_cost == &confusion && c == *str)
		enqueue(trie, &confusion, str+1, ofs+1, dist);
#endif
	    char tmp[2] = { c };
	    size_t _  = 0;
	    Penalty* next_cost = cur_cost->find_node(tmp, _);
	    if(next_cost) // TODO: penalty aware
		enqueue(trie, next_cost, str, ofs+1, dist);
	}

	void enqueue(general_fuzzy* entry, Penalty* cost, const char* str, size_t ofs, unsigned ndist) const
	{
	    if(ndist == dist) {
		if(!memo.insert(memo_key(entry, str)).second) 
		    return;
		feeder next = { cost, delayed, memo, results, str, ofs, ndist, limit, best_only };
		next.visit(entry);
		entry->template explore<feeder&>(next, 0);
	    } else if (ndist <= limit) {
		held record = { entry, cost, str, ofs };
		delayed[ndist].push_back(record);
	    }
	}

	void visit(general_fuzzy* entry) 
	{
	    for(size_t i=0; i < cur_cost->info.size(); i++) {
		unsigned ndist = dist + cur_cost->info[i].second;
		if(ndist > limit)
		    break;
		if(const char* nstr = prefixes(cur_cost->info[i].first, str)) 
		    enqueue(entry, &confusion, nstr, ofs, ndist);
	    }

            if(!entry->search_key || cur_cost != &confusion) return;

            unsigned const ndist = dist + entry->match_tail(str, ofs);

            if(ndist <= limit) {
                result record(entry,ndist);
                if(best_only && ndist < limit)
                    results.assign(1,record), limit = ndist;
                else if(best_only > 1 && !results.empty() && limit) //!
                    results.clear(), limit--;
                else
#if BEST_FIRST > 1
		    if(ndist == dist) {
			results.push_back(record);
		        memo.insert(memo_key(entry, 0)); // is this really wise? (pollutes memo)
		    } else {
			held record = { entry, 0, 0, ofs };
			delayed[ndist].push_back(record);
		    }
#else
                    results.push_back(record);
#endif
            }
	}

    private:
	~feeder() {} 
	friend class general_fuzzy;
    };

    void search_recursive(memo_table& memo, std::vector<result>& res, const char* str, size_t ofs, unsigned threshold, unsigned& limit, char best_only)
    {
	std::pair<typename memo_table::iterator, bool> lookup = memo.insert(memo_key(this, str));
	if(!lookup.second) return;

	typedef std::vector<typename feeder::held> bucket_vector;
	std::vector<bucket_vector> bucket_vec(1+limit); 
	bucket_vector* const bucket = bucket_vec.data();

	feeder recurse = { &confusion, bucket, memo, res, str, ofs, threshold, limit, best_only };
	recurse.visit(this);

	Trie::template explore<feeder&>(recurse, 0);

	while(++threshold <= limit)
	    for(size_t i=0; i < bucket[threshold].size(); ++i) {
		typename feeder::held& r = bucket[threshold][i];
		if(!memo.insert(memo_key(r.trie, r.str)).second) 
		    continue;
#if BEST_FIRST > 1
		if(r.str == 0) {
		    res.push_back(result(r.trie, threshold));
		    continue;
		}
#endif
		feeder next = { &confusion, bucket, memo, res, r.str, r.ofs, threshold, limit, best_only };
		next.visit(r.trie);
		r.trie->template explore<feeder&>(next, 0);
	    }
    }

    using Trie::search_key;
};

