#include <map>
#include <vector>
#include <utility>
#include <algorithm>
#include <string>
#include <limits.h>
#include "../trie/basis.cpp"
#include "../trie/impl/vector.cpp"
#include "../util/containers.h"

/* 
  Calculate a generalized n -> n edit distance between two given strings,
  using a 'toy' confusion matrix. 

  E.g. try "gothics" vs "gothicks" against "gofyxx"

 */

using namespace std;

typedef pair<string,unsigned int> cost;

typedef simple_trie< vector<cost> > penalty_trie;

typedef map<pair<string,string>, unsigned> memo_table;

struct symmetric_penalty_trie : penalty_trie {
    void add(string a, string b, unsigned cost)
    {
	(*this)[a.c_str()].push_back(make_pair(b.c_str(),cost));
	(*this)[b.c_str()].push_back(make_pair(a.c_str(),cost));
    }
} confusion;

// sort a node on weights: best transitions first
bool optimize_order(char, penalty_trie& trie, int)
{
    sort(trie.info.begin(), trie.info.end(), onkey(&cost::second));
    return true;
}

void init_costs()
{
    for(char c='a'; c <= 'z'; c++) 
	confusion.add("", string(1,c), 1);
    confusion.add(" ", "", 4);

    confusion.add("i", "y", 1);
    confusion.add("xx", "cks", 1);
    confusion.add("ice", "ise", 1);
    confusion.add("yze", "yse", 0);
    confusion.add("our", "or",  0);
    confusion.add("ough", "u",  1);
    confusion.add("eye", "aye",  1);
    confusion.add("th", "f",  2);
    confusion.walk(optimize_order);
}

bool prefixes(string a, string b)
{
    return a == b.substr(0,a.length());
}

void edit_distance(const char*, const char*, unsigned&);

unsigned edit_distance(const char* a, const char* b)
{
    unsigned d = INT_MAX;
    edit_distance(a,b,d);
    return d;
}

// delete prefixes from str to match prefixes in pattern
void word_cost(unsigned& distance, const char* pattern, const char* str)
{
    size_t ofs = 0;
    for(penalty_trie* node = &confusion; str[ofs] && (node = node->find_node(str, ofs)); ) {
	vector<cost>& repl = node->info;
	for(int i=0; i < repl.size(); ++i) {
	    string& pre = repl[i].first;
	    int w = repl[i].second;
	    if(w < distance && prefixes(pre, pattern)) {
		//printf("[%s]=>[%s]@%d\n", string(str).substr(0,ofs).c_str(), pre.c_str(), w);
		unsigned subdist = distance-w;
		edit_distance(pattern+pre.length(), str+ofs, subdist);
		if(subdist+w < distance)
		    distance = subdist+w;
	    }
	}
    }
}

memo_table memo;

void edit_distance(const char* pattern, const char* str, unsigned& dist)
{
    static int lvl;
    if(*str == 0 && *pattern == 0) {
	dist = 0;
	return;
    }

    pair<memo_table::iterator,bool> chk = memo.insert(make_pair(make_pair(pattern, str), INT_MAX));
    if(!chk.second) {
	if(chk.first->second >= dist)
	    return;
    }

    ++lvl;
    //printf("[%d:%s=?=%s]\n", lvl, pattern, str);

    if(*str == *pattern)
	edit_distance(str+1, pattern+1, dist);

    word_cost(dist, pattern, str);
    word_cost(dist, str, pattern);

    --lvl;

    chk.first->second = dist;
}

bool display(char c, const penalty_trie& lex, int ofs)
{
    for(int i=0; i<ofs*4; i++) putchar(' ');
    printf("[", lex.info.size());
    for(int i=0; i<lex.info.size(); i++)
	printf("%s%s", i?",":"",lex.info[i].first.c_str());
    printf("]");
    putchar(c);
    if(lex.search_key)
	printf("$\n");
    else
	printf(":\n");
    return true;
}

int main(int argc, char** argv)
{
    init_costs();
    if(!argv[1] || !argv[2]) {
	puts("enter two arguments");
	return 1;
    }
    #if 0
    confusion.walk(display);
    #endif
    printf("edit distance: %d\n", edit_distance(argv[1], argv[2]));
}
