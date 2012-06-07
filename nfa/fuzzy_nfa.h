#pragma once

/* A Levensthein automaton (for the normal Levenshtein distance)
   maximum width is fixed (default/optimal: 64bits) */

#include <bitset>
#include <cstring>
#include <cassert>

template<unsigned int Distance, size_t N=64>
struct fuzzy_nfa {
    std::bitset<N> pattern[256];
    unsigned short int width; 
    unsigned short int height;

    enum { max_distance = Distance };

    fuzzy_nfa(const char* text, int dist = max_distance) 
    : pattern(),  width(std::strlen(text)), height(dist)
    {
	std::bitset<N> one = 1;
	for(int i=0; i < width; ++i) 
	    pattern[text[i]&0xFF] |= one << i;
    }

    struct state {
	std::bitset<N> reg[max_distance+1];

	unsigned feed(const fuzzy_nfa& pattern, char c)
	{ return pattern.feed(*this, c, *this); }
    };

    state start()
    {
	std::bitset<N> pat;
	std::bitset<N> const one = 1;
	state s;
	for(int i=0; i <= height; i++)
	    s.reg[i] = (pat = pat<<1 | one); 
	return s;
    }

    bool accepts(const state& fsm, int dist) const
    { return fsm.reg[dist][width]; }
    bool accepts(const state& fsm) const
    { return accepts(fsm, height); }

    unsigned accepts_dist(const state& fsm) const
    {
	unsigned row = height+1;
	while(accepts(fsm,row-1) && --row)
	    ;
	return row;
    }

    bool deterministic(const state& fsm) const 
    {
	return fsm.reg[height].count() == 1;
    }

    unsigned eaten(const state& fsm, unsigned row=0) const
    {
	for(int i=width; i >= 0; --i)
	    if(fsm.reg[row][i]) return i;
	return 0;
    }

    inline std::bitset<N> clip(std::bitset<N> bits) const
    {
	return bits[width] = 0, bits;
    }

    unsigned feed(const state& fsm, char c, state& fsmnew) const
    {
	std::bitset<N> const& mask = pattern[c&0xFF];
	std::bitset<N> const* reg = fsm.reg;

	int i;
	for(i=height; i>0 && (reg[i-1].any()); --i) 
	    fsmnew.reg[i] = (reg[i]&mask) << 1 | reg[i-1] | clip(reg[i-1]) << 1;
	fsmnew.reg[i] = (reg[i]&mask) << 1;

	if(reg != fsmnew.reg) 
	    for(int j=0; j < i; ++j) fsmnew.reg[j].reset();
	reg = fsmnew.reg;

	int const best = i + reg[i].none();
	while(++i <= height)
	    fsmnew.reg[i] |= clip(reg[i-1]) << 1;

	return best;
    }

};
