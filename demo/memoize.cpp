#include <stdio.h>
#include <string.h>
#include "../util/memo.h"

#ifdef UNMEMOIZED
#define editdistance _editdistance
#else
int _editdistance(const char* a, const char* b);

memo_t<int(*)(const char*,const char*), int, std::pair<const char*,const char*> > editdistance = _editdistance;
#endif

 /* naive edit distance */

int _editdistance(const char* a, const char* b)
{
    if(!*a) return strlen(b);
    else if(!*b) return strlen(a);

    int del = 1+editdistance(a+1,b);
    int ins = 1+editdistance(a,b+1);
    int best = del<ins? del : ins;
    int repl = (*a!=*b) + editdistance(a+1,b+1);

    return repl < best? repl : best;
}

int main()
{
    printf("%d\n", editdistance("oewieweijjrwra", "qaalaafz"));
    printf("%d\n", editdistance("ijiwwoerwrhwihriwhrpiwhruhweprhwirhwprhpwiuhrwierwweorqwuhriwruwheirwewrerwerrhwuihrerwr", "ewerweohqwirhwirhwrhwrweoriwjerowhrhwirhwihebhrhwrbweaaaarruhwiurhwi"));
}
