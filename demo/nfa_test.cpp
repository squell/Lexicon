#include <iostream>
#include <string>
#include "../nfa/fuzzy_nfa.h"

using namespace std;

int main(int argc, char **argv)
{
    fuzzy_nfa<4> fsm(argc>1? argv[1] : "barbapappa");

    string word;
    while(cin >> word) {
	fuzzy_nfa<4>::state st = fsm.start();
	for(string::iterator p=word.begin(); p!=word.end(); ++p) {
	    st.feed(fsm, *p);
	}
	if(fsm.accepts(st)) {
	    cout << word << ":" << fsm.accepts_dist(st) << endl;
	}
    }
}
