#pragma once

/* Levenshtein automaton, implemented using the algorithm described by 
   G.Myers in Journal of the ACM vol. 46 p1-13; you stand no chance of
   understanding this without reading that paper.

   Limitations:
   - automaton doesn't support falling back to deterministic matching, because
     determining that from the state is hard, and it is not really needed
   - because the score can go down as well as up, the heuristic returned by
     feed() isn't informative enough for driving a best-first search

   Advantages:
   - the FSM state is a lot shorter, and constant in size wrt. edit distance
     and likewise update operations are constant wrt. edit distance

 */

#include <cstring>
#include <cassert>

template<class bits = unsigned long long>
struct myers_nfa {
    bits Peq[256];
    unsigned short int width; 
    unsigned short int height;

    myers_nfa(const char* text, unsigned dist=0xFFFF)
    : Peq(),  width(std::strlen(text)), height(dist)
    {
	for(int i=0; i < width; ++i) 
	    Peq[text[i]&0xFF] |= bits(1) << i;
    }

    struct state {
	bits Pv,Mv;
	unsigned short Score;
	unsigned short Heur;

	unsigned feed(const myers_nfa& pattern, char c)
	{ return pattern.feed(*this, c, *this); }
    };

    state start()
    {
	state s = { -1, 0, width, 0 };
	return s;
    }

    bool accepts(const state& fsm, int dist) const
    { return fsm.Score <= dist; }
    bool accepts(const state& fsm) const
    { return accepts(fsm, height); }

    unsigned accepts_dist(const state& fsm) const
    {
	return fsm.Score;
    }

    bool deterministic(const state& fsm) const 
    {
	return false;
    }

    unsigned eaten(const state& fsm, unsigned row=0) const
    {
	assert(!"myers_nfa::eaten called");
    }

    unsigned feed(const state& fsm, char c, state& fsmnew) const
    {
	bits const Pv = fsm.Pv;
	bits const Mv = fsm.Mv;
	bits const Cf = bits(1) << width-1;
	bits const Eq = Peq[c&0xFF];
	bits const Xv = Eq | Mv;
	bits const Xh = (Eq&Pv)+Pv ^ Pv | Eq;
	bits       Ph = Mv | ~(Xh|Pv);
	bits const Mh = Pv & Xh;
	if(Ph & Cf) 
	    fsmnew.Score = fsm.Score+1;
	else if(Mh & Cf) 
	    fsmnew.Score = fsm.Score-1;
	else
	    fsmnew.Score = fsm.Score;
	Ph = (Ph << 1) | 1;
	fsmnew.Pv = (Mh << 1) | ~(Xv|Ph);
	fsmnew.Mv = Ph & Xv;
	fsmnew.Heur = (fsmnew.Score>fsm.Heur+width)? fsmnew.Score-width : fsm.Heur;
	return fsmnew.Heur;
    }
};

