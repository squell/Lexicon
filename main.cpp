#include <iostream>
#include <iomanip>
#include <fstream>
#include "environ.h"
#include "util/padding.h"
#include "nfa/fuzzy_nfa.h"
#include "nfa/slide_nfa.h"
#include "nfa/myers_nfa.h"
#include "nfa/limex_nfa.h"
#include "nfa/weighted_nfa.h"
#include "trie/impl/base.h"
#include "trie/impl/list.cpp"
#include "trie/impl/tree.cpp"
#include "trie/impl/vector.cpp"
#include "trie/impl/dumb.cpp"
#include "trie/basis.cpp"
#include "trie/turbo.cpp"
#include "trie/serialize.cpp"
#include "trie/fuzzy.cpp"
#include "trie/direct_fuzzy.cpp"
#include "trie/general_fuzzy.cpp"

#ifndef FUZZY
#  define FUZZY 1
#endif

// fuzzy patricia
// fix static_cast<>
// fix delete
// fix gnu hack iter_swap
// ? hacked trie<>
// ? tweak io to make it universal
// ? glue char8
// ? threads
// ? fuzzy hardcode/traits

typedef char key;
//typedef char_ptr key;
//typedef char_store<8> key;

//typedef general_fuzzy<simple_trie<std::string,Vector,key> > Lexicon;
//typedef general_fuzzy<trie<void,Vector,false,key> > Lexicon;
//typedef fuzzy<trie<void,Vector,false,key>, fuzzy_nfa<8> > Lexicon;
//typedef direct_fuzzy<simple_trie<std::string,Vector,key>, penalty_file > Lexicon;
//typedef direct_fuzzy<trie<void,Vector,false,key>, penalty_file > Lexicon;
//typedef direct_fuzzy<simple_trie<std::string,Vector,key>, penalty_file > Lexicon;
///typedef fuzzy<trie<void,Vector,false,key>, fuzzy_nfa<4> > Lexicon;
//typedef fuzzy<trie<void,Vector,false,key>, g_nfa<64, g_nfa_file> > Lexicon;
//typedef direct_fuzzy<simple_trie<void,Vector,key> > Lexicon;
//typedef fuzzy<trie<void,Vector,false,key>, g_nfa<> > Lexicon;
//typedef fuzzy<trie<void,Vector,false,key>, slide_nfa<4> > Lexicon;
//typedef simple_trie<void,Array,key> Lexicon;
//typedef trie<void,LinkedList,true,key> Lexicon;
//typedef trie<void,LinkedList,true,key> Lexicon;
//typedef simple_trie<void,Vector,char_store<8> > Lexicon;
//typedef trie<void,CompactVector,true,char> Lexicon;
//typedef trie<void,AVLTree,true,char> Lexicon;
//typedef simple_trie<void,Vector,char> Lexicon;
typedef fuzzy< simple_trie<void,LinkedList,char_ptr> > Lexicon;
//typedef direct_fuzzy<simple_trie<void,LinkedList,char> > Lexicon;
//typedef direct_fuzzy<simple_trie<void,LinkedList,char>, penalty_file > Lexicon;
//typedef fuzzy<simple_trie<std::string,LinkedList,char>, fuzzy_nfa<FUZZY> > Lexicon;

struct statistics {
    static size_t arity[256];
    static size_t nodes[256];
    static size_t total_nodes;
    static size_t allocated;
    static size_t needed;
    static size_t padding;

    template<class T, class K>
    bool operator()(K k, const T& lex, int ofs)
    {
	arity[ofs] += lex.arity();
	nodes[ofs]++;
	total_nodes++;
	allocated += memused(lex)+memused(k); 
	needed    += memused(lex, smallsize)+memused(k,smallsize); 
	padding   += ::padding<T>::amount;
	return 1;
    }

    static inline size_t smallsize(size_t t)
    {
	return t;
    }
};

size_t statistics::arity[];
size_t statistics::nodes[];
size_t statistics::total_nodes;
size_t statistics::allocated;
size_t statistics::needed;
size_t statistics::padding;

using namespace std;

int main(int argc, char** argv)
{
    srand(time(0));
    //penalty_file HJ("confusion.txt");
    unsigned distance = 4;
    unsigned nresults = 10;
    unsigned mode = 2;
    unsigned long N = 0;
    fstream src(argv[1]);
    string s;
    {
    Lexicon* lexicon;
    if(!argv[2] || string(argv[2]) != "+") {
	lexicon = new Lexicon;
	cout << tstamp() << "Reading in words" << endl;
	while(getline(src, s))
	    //lexicon->insert(s.c_str(),0).info = s;
	    lexicon->insert(s.c_str());
	//cout << tstamp() << "Sorting" << endl;
	//lexicon->sort();
	cout << tstamp() << "Optimizing" << endl;
	lexicon->optimize();
    } else {
	cout << tstamp() << "Reading" << endl;
	unserialize::read(src, lexicon);
	argv++;
	if(!src) {
	    cout << tstamp() << "Whoopee!" << endl;
	    return 0;
	}
    }
    if(argv[3]&&string(argv[3]) == "+") {
        ofstream out(argv[2]);
        cout << tstamp() << "Writing" << endl;
	serialize::write(out, lexicon);
    } else {
	turbo<Lexicon::trie_type> lexicon_fast(lexicon);
        cout << tstamp() << "Matching" << endl;
	if(argv[2]) {
	    ifstream test(argv[2]);
#if !FUZZY
	    while(getline(test, s))
		if(lexicon_fast->search(s.c_str())) N++;
#else
	    while(getline(test, s)) {
		//cout << N << "\r" << flush;
		N += lexicon->search_fuzzy(s.c_str(), distance, mode).size();
		//N += lexicon->search_fuzzy(s.c_str(), distance, mode, HJ).size();
		//for(int i=0; i<vec.size(); ++i) cout << vec[i].first->search_key << endl;
	    }
#endif
	} else 
	    while(cout << "> ", getline(cin, s) && !s.empty()) {
#if !FUZZY
		tstamp();
		const Lexicon::trie_type* res = lexicon_fast->search(s.c_str());
		if(res)
		    cout << tstamp() << "! "<< res->search_key << endl;
		else
		    cout << tstamp() << "? "<< s << endl;
#else
		if(s.empty()) continue; 
		if(s[0] == '@') { cout << s.substr(1) << endl; continue; }
		if(s[0] == '#') { nresults = atoi(s.c_str()+1); continue; }
		if(s[0] == '^') { mode = atoi(s.c_str()+1); continue; }
		if(s[0] == '$') { distance = atoi(s.c_str()+1); continue; } 

		tstamp();
		const Lexicon::trie_type* res = lexicon_fast->search(s.c_str());
		if(res) {
		    //cout << tstamp() << "!" << res->search_key << "=>" << res->info << endl;
		    cout << tstamp() << "! "<< res->search_key << endl;
		    //cout << tstamp() << "! "<< res->info << endl;
		    if(mode) continue;
		} else {
		    cout << tstamp() << "? "<< s << endl;
		}

		vector<Lexicon::result> vec = lexicon->search_fuzzy(s.c_str(), distance, mode);
		//vector<Lexicon::result> vec = lexicon->search_fuzzy(s.c_str(), std::max(s.length(), (size_t)distance), mode);
		//vector<Lexicon::result> vec = lexicon->search_fuzzy(s.c_str(), distance, mode, HJ);
		cout << tstamp() << "#" << vec.size() << endl;
		for(int i=0; i < vec.size(); ++i)
		    if(i < nresults)
			//cout << "$" << vec[i].second << " " << vec[i].first->search_key << "=>" << vec[i].first->info << endl;
			cout << "$" << vec[i].second << " " << vec[i].first->search_key << endl;
			//cout << "$" << vec[i].second << " " << vec[i].first->info << endl;
		    else {
			cout << "..." << endl;
			break;
		    }
#endif
	    }
    }

    cout << tstamp() << "Done" << endl;
    statistics info;
    lexicon->walk(info);
    cout << tstamp() << "Nodes: " << info.total_nodes << endl;
    #if 0
    for(int i=0; i<64; i++)
	cout << tstamp() << "@ " << left << setw(2) << i << ": " << right << setw(8) << info.nodes[i] << " nodes, " << setw(4) << setprecision(1) << fixed << 1.0*info.arity[i]/info.nodes[i] << " avg. arity" << endl;
    #endif
    cout << tstamp() << "Memory: " << info.allocated/1024 << "kb used, " << info.needed/1024 << "kb needed (" << int(100.0*info.needed/info.allocated+0.5) << "%), " << info.padding/1024 << "kb padding (" << int(100.0*info.padding/info.needed+0.5) << "%)" << endl;
    }
    cout << tstamp() << "Finished " << N << endl;
}
