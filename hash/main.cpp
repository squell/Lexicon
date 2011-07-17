#ifndef HASH
#define HASH fixed
#endif
#include "../environ.h"

#define foo_str1(arg) #arg
#define foo_str2(a,b) foo_str1(a ## b)
#define foo_str3(a,b) foo_str2(a, b)
#include foo_str3(HASH, _table.cpp)

using namespace std;

template<class HashTable>
void stats(HashTable& lex)
{
    size_t idle = 0;
    size_t size = 0;
    size_t maxx = 0;
    for(size_t h = 0; h < lex.buckets(); ++h) {
	idle += lex.tab[h].empty();
	size += lex.tab[h].size();
	maxx = max(maxx, lex.tab[h].size());
    }
    size_t bits = -1;
    for(size_t h = lex.buckets(); h; h >>= 1) 
	++bits;
    cout << "Hash size in bits: " << bits << endl;
    cout << "Average bucket size: " << float(size)/(lex.buckets()-idle) << endl;
    cout << "Unused buckets: " << idle << " (" << setprecision(1) << (idle)*100/lex.buckets() << "%)" << endl;
    cout << "Maximal bucket: " << maxx << endl;
}

int main(int argc, char** argv)
{
    unsigned long N = 0;
    fstream src(argv[1]);
    fstream test(argv[2]);
    string s;
    {
    static hash_table<int> real_lexicon;
    hash_table<int>* lexicon = &real_lexicon;
    cout << tstamp() << "Reading in words" << endl;
    while(getline(src, s))
	lexicon->insert(s.c_str(),0);

    cout << tstamp() << "Matching" << endl;
    while(getline(test, s))
	if(lexicon->search(s.c_str())) N++;
    cout << tstamp() << "Done" << endl;
    stats(*lexicon);
    cout << tstamp() << "Memory used: " << memused(*lexicon)/1024 << "kb" << endl;
    }
    cout << tstamp() << "Finished " << N << endl;
}
