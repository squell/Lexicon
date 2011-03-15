#pragma once

/* An attempt at a Levensthein automaton with weights; looses its appeal; 
   char update operations are no longer constant wrt. the pattern and 
   becomes rather complex. */

#include <bitset>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <fstream>

struct weighted_nfa_matrix {
    unsigned char table[256][256];
    unsigned const char& operator()(char a, char b) const
    {
	return table[b&0xFF][a&0xFF];
    }
};

struct weighted_nfa_demo : weighted_nfa_matrix {
    weighted_nfa_demo()
    {
	for(int i=0; i < 256; i++) 
	    for(int j=0; j < 256; j++)
		table[i][j] = i==j? 0 : 2;
	table['e']['a']=1;
    }
};

struct weighted_nfa_file : weighted_nfa_matrix
{
    weighted_nfa_file(const char* fname)
    {
        std::ifstream stored(fname);
        for(int i=0; i < 256; i++)
            for(int j=0; j < 256; j++) {
                size_t d;
                stored >> d;
                table[i][j] = d;
            }
        if(!stored.good())
            throw std::ios::failure("nfa_file: can't open file");
    }
};

struct weighted_nfa_fixed {
    unsigned char operator()(char a, char b) const
    {
	return a != b;
    }
};

template<size_t N=64, class cost_table=weighted_nfa_fixed>
struct weighted_nfa {
    cost_table D;
    const char* pattern;
    unsigned short int width; 
    unsigned short int height;

    enum { max_distance = 256 };

    weighted_nfa(const char* text, int dist, const cost_table& costs = cost_table()) 
    : D(costs), pattern(text),  width(std::strlen(text)), height(dist)
    {
    }

    struct state {
	unsigned char col[N];
	unsigned short shift, stop;

	unsigned feed(const weighted_nfa& pattern, char c)
	{ return pattern.feed(*this, c, *this); }
    };

    state start()
    {
	state s;

	size_t Max = std::min(N, (size_t)width+1);
	int acc = 0;
	s.stop = 0;
	for(unsigned i=0; i < Max; ++i) {
	    s.col[i] = acc;
	    if(acc <= height) s.stop = i;
	    acc = std::min(height+1, acc + D(0,pattern[i]));
	}
	s.shift = 0;

	return s;
    }

    bool accepts(const state& fsm, int dist) const
    { return fsm.stop+fsm.shift == width && fsm.col[fsm.stop] <= dist; }
    bool accepts(const state& fsm) const
    { return accepts(fsm, height); }

    unsigned accepts_dist(const state& fsm) const
    {
	return fsm.stop+fsm.shift == width? fsm.col[fsm.stop] : height+1;
    }

    bool deterministic(const state& fsm) const 
    {
	assert(!"implement");
    }

    unsigned eaten(const state& fsm, unsigned row=0) const
    {
	assert(!"implement");
    }

    static int min4(int a, int b, int c, int d)
    {
	return std::min(std::min(a,b),std::min(c,d));
    }

    unsigned feed(const state& fsm, char c, state& fsmnew) const
    {
	/*
	printf("{%s}\n", pattern+fsm.shift);
	printf("[scan:%c]",c);
	for(int i=0; i <= fsm.stop; ++i) 
	    printf("%d ",fsm.col[i]);
	printf("\n");
	*/

	int best, old_left, new_left, nshift;
	old_left = fsm.col[0];
	new_left = best = old_left + D(c,0);

	int j=0;
	if(new_left > height) 
	    nshift = fsm.shift+1;
	else {
	    nshift = fsm.shift;
	    fsmnew.col[j++] = new_left;
	}

	for(int i=1; i <= fsm.stop; ++i) {
	    char const t = pattern[i-1+fsm.shift];
	    new_left = min4(
		old_left   + D(c,t),
		fsm.col[i] + D(c,0),
		new_left   + D(0,t),
		height+1
	    );
	    old_left = fsm.col[i];
	    fsmnew.col[j++] = new_left;
	    if(best > new_left)
		best = new_left;
	}

	if(fsm.stop+fsm.shift < width) {
	    char t = pattern[fsm.stop+fsm.shift];
	    new_left = std::min(old_left+D(c,t), new_left+D(0,t));
	    if(best > new_left)
		best = new_left;
	    if(new_left <= height && j < N-1)
		fsmnew.col[j++] = new_left;
	} else if(new_left > height) 
	    j--; 

	fsmnew.shift = nshift;
	fsmnew.stop  = std::max(0,j-1);

	/*
	printf("{%s}\n", pattern+fsmnew.shift);
	printf("[scan:%c]",c);
	for(int i=0; i <= fsmnew.stop; ++i) 
	    printf("%d ",fsmnew.col[i]);
	printf("\n");
	*/

	return best;
    }

};
