#pragma once

/* A Levensthein automaton (for the normal Levenshtein distance) 
   maximum width of the automaton is limited by machine word size */

#include <cstring>
#include <cassert>
#include <math.h>

template<unsigned int max_distance, size_t N=64> struct fuzzy_nfa;

template<unsigned int max_distance>
struct fuzzy_nfa<max_distance,64> {
    unsigned long long pattern[256];
    unsigned short int width; 
    unsigned short int height;

    fuzzy_nfa(const char* text, int dist = max_distance) 
    : width(std::strlen(text)), height(dist), pattern()
    {
	for(int i=0; i < width; ++i) 
	    pattern[text[i]&0xFF] |= 1ULL << i;
    }

    struct state {
	unsigned long long reg[max_distance+1];

	unsigned feed(const fuzzy_nfa& pattern, char c)
	{ return pattern.feed(*this, c, *this); }
    };

    state start()
    {
	state s;
	for(int i=0; i <= height; i++)
	    s.reg[i] = (2ULL << i) - 1; 
	return s;
    }

    bool accepts(const state& fsm, int dist) const
    { return fsm.reg[dist] & 1ULL << width; }
    bool accepts(const state& fsm) const
    { return accepts(fsm, height); }

    unsigned accepts_dist(const state& fsm) const
    {
	unsigned row = height+1;
	while(accepts(fsm,row-1) && --row)
	    ;
	return row;
    }

    int deterministic(const state& fsm) const 
    {
	unsigned long long const clip = (1ULL<<width+1)-1;
	unsigned long long mask = fsm.reg[height]&clip;

	return !(mask & mask-1);

	return ((mask & 0xFFFFFFFF00000000ULL) != 0) << 5
	     | ((mask & 0xFFFF0000FFFF0000ULL) != 0) << 4
	     | ((mask & 0xFF00FF00FF00FF00ULL) != 0) << 3
	     | ((mask & 0xF0F0F0F0F0F0F0F0ULL) != 0) << 2
	     | ((mask & 0xCCCCCCCCCCCCCCCCULL) != 0) << 1
	     | ((mask & 0xAAAAAAAAAAAAAAAAULL) != 0);
    }

    unsigned eaten(const state& fsm, unsigned row=0) const
    {
	unsigned long long const clip = (1ULL<<width+1)-1;
	unsigned long long num  = fsm.reg[row]&clip;
	
	return ilogb(num);
    }

    unsigned min_pos(const state& fsm) const 
    {
	unsigned long long const clip = (1ULL<<width+1)-1;
	int row = 0;
	for( ; row <= height; ++row) 
	    if(fsm.reg[row]&clip) return row;
	return row;
    }

    unsigned max_pos(const state& fsm) const
    {
	unsigned long long const clip = (1ULL<<width+1)-1;
	unsigned long long mask = 0;

	int row = height+1;
	while(row > 0 && (fsm.reg[row-1]&clip)<<1 > mask) 
	    mask = fsm.reg[--row]&clip;

	return row;
    }

    unsigned best_pos(const state& fsm) const
    {
	#if 1
	unsigned long long mask = (1ULL<<width+1)-1;
	int row;
	for(row=0; row < height; ++row) {
	    unsigned long long reg = fsm.reg[row];
	    mask &= ~(reg<<1 | reg);
	    if(reg > (fsm.reg[row+1]&mask)) break;
	}
	
	return row;
	#else
	unsigned long long const mask = (1ULL<<width+1)-1;
	int row;
	for(row=0; row < height; ++row) {
	    if(row&mask) break;
	}
	return row;
	#endif
    }

    unsigned feed(const state& fsm, char c, state& fsmnew) const
    {
	unsigned long long const clip = (1ULL<<width+1)-1;
	unsigned long long mask = pattern[c&0xFF];

	unsigned long long const* reg = fsm.reg;

	int i;
	for(i=height; i>0 && (reg[i-1]&clip); --i) 
	    fsmnew.reg[i] = (reg[i]&mask) << 1 | reg[i-1] | reg[i-1] << 1;
	fsmnew.reg[i] = (reg[i]&mask) << 1;

	if(reg != fsmnew.reg) 
	    for(int j=0; j < i; ++j) fsmnew.reg[j] = 0;
	reg = fsmnew.reg;

	int const best = i + !(reg[i]&clip);
	while(++i <= height)
	    fsmnew.reg[i] |= reg[i-1] << 1;

	return best;
    }

};
