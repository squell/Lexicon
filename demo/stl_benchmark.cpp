#include <iostream>
#include <iomanip>
#include <fstream>
#include "../environ.h"
#ifdef HASHMAP
#include <tr1/unordered_set>
using namespace std::tr1;
#else
#include <set>
#define unordered_set set
#endif

/* The 'main' of the hash program implemented using the STL. 
   When HASHMAP is defined, should have comparable performance to hash/main.cpp. 

   This gives a reference benchmark, of sorts. */

using namespace std;

int main(int argc, char** argv)
{
    unsigned long N = 0;
    fstream src(argv[1]);
    fstream test(argv[2]);
    string s;
    {
    unordered_set<string> lexicon;
    cout << tstamp() << "Reading in words" << endl;
    while(getline(src, s))
	lexicon.insert(s.c_str());

    cout << tstamp() << "Matching" << endl;
    while(getline(test, s))
	if(lexicon.find(s) != lexicon.end()) N++;
    cout << tstamp() << "Done" << endl;
    }
    cout << tstamp() << "Finished " << N << endl;
}
