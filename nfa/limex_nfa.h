#pragma once

/* Levensthein automaton variation:

   Calculates regular Levenshtein distance, but the automaton can be
   initialized using limited regular expression:

   - character choice: [abcde] and .
   - repetition: +

   if ZEROTRANS is true, also:
   - optional:   ?
   - repetition: *

*/

#include <bitset>
#include <cstring>
#include <cassert>

#define ZEROTRANS 1

template<unsigned int max_editdistance, size_t N=64>
struct limex_nfa {
    std::bitset<N> pattern[256];
    std::bitset<N> repeat;
#if ZEROTRANS
    std::bitset<N> skip;
#endif
    unsigned short int width; 
    unsigned short int height;

    limex_nfa(const char* text, int dist = max_editdistance) 
    : pattern(),  width(0), height(dist)
    {
	std::bitset<N> one = 1;
	for(int i=0; text[i]; ++i) {
	    switch(text[i]) {
#if ZEROTRANS
	    case '?':
	    case '*':
		skip |= one << width-1;
		if(text[i] == '?') break;
#endif
	    case '+':
		repeat |= one << width-1;
		break;
	    case '[':
		bool neg;
		if(neg = text[i+1] == '^') ++i;
		while(text[i+1] && text[++i] != ']') {
		    int b = text[i]&0xFF;
		    int e = text[i+1]=='-'&&text[i+2]!=']'? text[i+=2]&0xFF : b;
		    for(int c=0; c < 256; ++c)
			if((b<=c && c<=e) != neg)
			    pattern[c] |= one << width;
		} 
		width++;
		break;
	    case '.':
		for(int c=0; c < 256; ++c)
		    pattern[c] |= one << width;
		width++;
		break;
	    case '\\':
		++i;
	    default:
		pattern[text[i]&0xFF] |= one << width++;
	    }
	}
    }

    struct state {
	std::bitset<N> reg[max_editdistance+1];

	unsigned feed(const limex_nfa& pattern, char c)
	{ return pattern.feed(*this, c, *this); }
    };

    state start()
    {
	std::bitset<N> pat;
	std::bitset<N> const one = 1;
	state s;
	for(int i=0; i <= height; i++) {
	    pat = pat<<1 | one;;
#if ZEROTRANS
	    pat |= (pat&skip) << 1;
#endif
	    s.reg[i] = pat;
	}
	for(int i=0; i <= height; i++)
	    s.reg[i] |= (s.reg[i]&skip) << 1;
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
	    fsmnew.reg[i] = (reg[i]&mask) << 1 | reg[i-1] | clip(reg[i-1]) << 1 | (reg[i]&repeat&mask);
	fsmnew.reg[i] = (reg[i]&mask) << 1 | (reg[i]&repeat&mask);

	if(reg != fsmnew.reg) 
	    for(int j=0; j < i; ++j) fsmnew.reg[j].reset();
	reg = fsmnew.reg;

	int const best = i + reg[i].none();
#if ZEROTRANS
	fsmnew.reg[i] |= (fsmnew.reg[i]&skip) << 1;
#endif
	while(++i <= height) 
#if ZEROTRANS
	    fsmnew.reg[i] |= clip(reg[i-1]) << 1,
	    fsmnew.reg[i] |= (fsmnew.reg[i]&skip) << 1;
#else
	    fsmnew.reg[i] |= clip(reg[i-1]) << 1;
#endif

	return best;
    }

};
