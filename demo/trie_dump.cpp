#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include "../trie/impl/list.cpp"
#include "../trie/basis.cpp"
#include "../trie/serialize.cpp"

typedef simple_trie<void,LinkedList,char_store<4> > Lexicon;

using namespace std;

template<class T> const char* str_repr(const T& s)   
{
    if((void*)s.data == (void*)&s.data) {
        static char data[128];
	sprintf(data, "%.*s", sizeof s.data, s.data);
	return data;
    } else {
	return s.data; 
    }
}
template<> const char* str_repr(const char*const& s) { return s; }
template<> const char* str_repr(const bool&)         { return ""; }
template<> const char* str_repr(const char& c)       { static char data[2] = {0,0}; data[0]=c; return data; }

bool display(Lexicon::key_type c, const Lexicon::trie_type& lex, int ofs)
{
    for(int i=0; i<ofs*4; i++) putchar(' '); 
    printf("%s", str_repr(c));
    if(lex.search_key)
	printf("$%s\n", str_repr(lex.search_key));
    else
	printf(":\n");
    return true;
}

int main(int, char** argv)
{
    if(argv[1]) {
	ifstream file(argv[1]);
	Lexicon* lex = 0;
	if(unserialize::read(file, lex)) {
	    lex->walk(display);
	}
    } else {
	Lexicon lex;
	lex.insert("aap");
	lex.insert("arfman");
	lex.insert("burp");
	lex.insert("closer");
	lex.insert("koekjes");
	lex.insert("koe");
	lex.insert("koekenpan");
	lex.walk(display); 
    }
    return 0;
}
