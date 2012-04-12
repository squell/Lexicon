#pragma once

/* A Levensthein automaton (for the normal Levenshtein distance)
   the automaton 'slides' along; so the state can remain small, 
   but it is complicated thing. */

#include <bitset>
#include <cstring>
#include <cassert>
#include <math.h>

// potentially helps the optimizer
#define WEIRD_OPT 1

template<unsigned int Distance, size_t K=128, class Bitstate=unsigned int>
struct slide_nfa {
    std::bitset<K> pattern[256];
    short int width; 
    short int height;
    
    enum { max_distance = Distance };
    enum { W = sizeof(Bitstate)*8 };

    slide_nfa(const char* text, int dist = max_distance) 
    : width(std::strlen(text)), height(dist), pattern()
    {
	std::bitset<K> one = 1;
	for(int i=0; i < width; ++i) 
	    pattern[text[i]&0xFF] |= one << i;
    }

    struct state {
	Bitstate reg[max_distance+1];
	short shift;

	unsigned feed(const slide_nfa& pattern, char c)
	{ return pattern.feed(*this, c, *this); }
    };

    state start()
    {
	state s;
	for(int i=0; i <= height; i++)
	    s.reg[i] = (2ULL << i) - 1; 
	s.shift = 0;
	return s;
    }

    bool accepts(const state& fsm, int dist) const
    { return fsm.reg[dist] & 1ULL << width-fsm.shift && width < fsm.shift+W; }
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
	return !(fsm.reg[height] & fsm.reg[height]-1);
    }

    unsigned eaten(const state& fsm, unsigned row=0) const
    {
	return ilogb(fsm.reg[row]) + fsm.shift;
    }

    static Bitstate bits(int n) 
    {
	return (1ULL<<(n<W?n:W))-1;
    }

    unsigned feed(const state& fsm, char c, state& fsmnew) const
    {
	Bitstate const clip = bits(width-fsm.shift+1);
	#if 1
	std::bitset<K> const& enabled = (pattern[c&0xFF]>>fsm.shift) &= clip;
	#else
	std::bitset<W> const& enabled = ((std::bitset<W>&)pattern[c&0xFF]>>fsm.shift) &= clip;
	#endif
	Bitstate const mask = enabled.to_ulong();

	Bitstate const* reg = fsm.reg;

	bool const shl = fsm.reg[height]&(1ULL<<W-1) && width-fsm.shift>=W;

	int i;
	if(WEIRD_OPT && !shl) {
	    for(i=height; i>0 && (reg[i-1]&clip); --i) 
		fsmnew.reg[i] = (reg[i]&mask) << 1-shl | reg[i-1]>>shl | reg[i-1] << 1-shl;
	    fsmnew.reg[i] = (reg[i]&mask) << !shl;
	} else {
	    for(i=height; i>0 && (reg[i-1]&clip); --i) 
		fsmnew.reg[i] = (reg[i]&mask) << 1-shl | reg[i-1]>>shl | reg[i-1] << 1-shl;
	    fsmnew.reg[i] = (reg[i]&mask) << !shl;
	}

	if(reg != fsmnew.reg) 
	    for(int j=0; j < i; ++j) fsmnew.reg[j] = 0;
	reg = fsmnew.reg;

	int const best = i + !(reg[i]&clip);
	if(WEIRD_OPT && !shl) 
	    while(++i <= height)
		fsmnew.reg[i] |= reg[i-1] << 1-shl;
	else
	    while(++i <= height)
		fsmnew.reg[i] |= reg[i-1] << 1-shl;

	fsmnew.shift = fsm.shift+shl;
	return best;
    }

};
