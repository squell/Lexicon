#pragma once

#include <utility>
#include <algorithm>
#include <vector>

#include "../nfa/fuzzy_nfa.h"
#include "impl/base.h"

// local best first technique
// 5 - full best first
// 4 - global buckets
// 3 - local buckets
// 2 - sorted list
// 1 - low hanging fruit
// 0 - none
#ifndef SEARCH_ORDER
#define SEARCH_ORDER 4
#endif

template<class Trie, class nfa=fuzzy_nfa<8> >
struct fuzzy : Trie {
    typedef typename Trie::link link;
    typedef std::pair<const fuzzy*,unsigned> result;

    typedef typename nfa::state nfastate;

    static unsigned match(bool, const nfa& fsm, const nfastate& state)
    { return fsm.accepts_dist(state); }

    /*
    static unsigned match(const char* str, const nfa& fsm, nfastate state)
    { 
	while(char c = *str++)
	    if(state.feed(fsm, c) > fsm.height) return fsm.height+1;
	return fsm.accepts_dist(state);
    }
    */
    static unsigned match(const char* str, const nfa& fsm, const nfastate& state)
    { 
	if(char c = *str++) {
	    nfastate nstate;
	    if(fsm.feed(state, c, nstate) > fsm.height) return -1;
	    while(char c = *str++)
		if(nstate.feed(fsm, c) > fsm.height) return -1;
	    return fsm.accepts_dist(nstate);
	} else
	    return fsm.accepts_dist(state);
    }

    unsigned match_tail(const nfa& fsm, const nfastate& state, size_t ofs=0)
    {
	if(Trie::full_key)
	    return match(search_key+ofs, fsm, state);
	else
	    return match(search_key, fsm, state);
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

    std::vector<result> search_fuzzy(const char* str, unsigned limit=nfa::max_distance, char mode=true)
    {
	std::vector<result> res;
	search_fuzzy(res, str, limit, mode);
	return res;
    }

    void search_fuzzy(std::vector<result>& res, const char* str, unsigned limit=nfa::max_distance, char mode=true)
    {
	nfa fsm(str,limit);
	nfastate init = fsm.start(); 
	search_nfa(res, fsm, init, mode);
    }

    struct feeder {
#if SEARCH_ORDER
	#if SEARCH_ORDER > 2
	struct held { fuzzy* trie; nfastate state; size_t ofs; };
	#else
	struct held { fuzzy* trie; nfastate state; unsigned dist; };
	#endif
	std::vector<held>* const delayed;
#endif
	std::vector<result>& results;
	nfa& fsm;
	nfastate const state;
	size_t const ofs;
	unsigned const dist;
	char const best_only;

	void operator()(char c, Trie& entry) const
	{ 
	    fuzzy* trie = static_cast<fuzzy*>(&entry);
	    nfastate next_state;
#if SEARCH_ORDER
	    unsigned ndist;
	    if((ndist=fsm.feed(state, c, next_state)) <= fsm.height) {
		if(ndist == dist) {
		    #if SEARCH_ORDER > 4
		    feeder next = { delayed, results, fsm, next_state, ofs+1, dist, best_only };
		    next.visit(trie);
		    trie->template explore<feeder&>(next, 0);
		    #else
		    trie->search_nfa(results, fsm, next_state, best_only, ofs+1, dist);
		    #endif
		} else {
		    #if SEARCH_ORDER > 2
		    held cont = { trie, next_state, ofs+1 };
		    delayed[ndist].push_back(cont);
		    #else
		    held cont = { trie, next_state, ndist };
		    delayed->push_back(cont);
		    #endif
		}
	    }
#else
	    if(fsm.feed(state, c, next_state) <= fsm.height)
		trie->search_nfa(results, fsm, next_state, best_only, ofs+1);
#endif
	}

	void visit(fuzzy* const entry) const
	{
	    if(!entry->search_key) return;

	    unsigned dist = entry->match_tail(fsm, state, ofs);

	    if(dist <= fsm.height) {
		result record(entry,dist);
		if(best_only && dist < fsm.height) 
		    results.assign(1,record), fsm.height = dist;
		else if(best_only > 1 && !results.empty())
		    results.clear(), fsm.height--;
		else
		    results.push_back(record);
	    }
	}
    private:
	~feeder() {} 
	friend class fuzzy;
    };

    void search_nfa(std::vector<result>& results, nfa& fsm, nfastate const& state, char best_only, size_t ofs=0, size_t threshold=0)
    {
#if SEARCH_ORDER
	/* one too many, since we never insert at the same distance level */
	std::vector<typename feeder::held> bucket[1+nfa::max_distance*(SEARCH_ORDER>2)]; 
	feeder recurse = { bucket, results, fsm, state, ofs, threshold, best_only };
#else
	feeder recurse = { results, fsm, state, ofs, threshold, best_only };
#endif
	recurse.visit(this);

	Trie::template explore<feeder&>(recurse, 0);
#if SEARCH_ORDER > 2
	while(++threshold <= fsm.height)
	    for(size_t i=0; i < bucket[threshold].size(); ++i) {
		typename feeder::held& r = bucket[threshold][i];
		#if SEARCH_ORDER > 3
		feeder next = { bucket, results, fsm, r.state, r.ofs, threshold, best_only };
		next.visit(r.trie);
		r.trie->template explore<feeder&>(next, 0);
		#else
		r.trie->search_nfa(results,fsm,r.state,best_only,ofs+1,threshold);
		if(threshold > fsm.height) break;
		#endif
	    }
#elif SEARCH_ORDER
	#if SEARCH_ORDER > 1
	std::sort(recurse.delayed->begin(), recurse.delayed->end(), onkey(&feeder::held::dist));
	#endif
	for(size_t i=0; i < recurse.delayed->size(); ++i) {
	    typename feeder::held& r = (*recurse.delayed)[i];
	    if(r.dist <= fsm.height)
		r.trie->search_nfa(results,fsm,r.state,best_only,ofs+1,r.dist);
	    #if SEARCH_ORDER > 1
	    else
		break;
	    #endif
	}
#endif
    }

    using Trie::search_key;
};
