#include <iostream>
#include <string>
#include "../nfa/fuzzy_nfa.h"
#include "../nfa/myers_nfa.h"
#include "../nfa/limex_nfa.h"

using namespace std;

int main(int argc, char **argv)
{
    typedef limex_nfa<4> nfa;
    nfa fsm(argc>1? argv[1] : "[bp]ar*[bp]a[pf]a[pf][pb]a+");

    string word;
    while(cin >> word) {
	nfa::state st = fsm.start();
	for(string::iterator p=word.begin(); p!=word.end(); ++p) {
	    st.feed(fsm, *p);
	}
	if(fsm.accepts(st)) {
	    cout << word << ":" << fsm.accepts_dist(st) << endl;
	}
    }
}
