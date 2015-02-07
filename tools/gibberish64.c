#include <time.h>
#include <stdlib.h>

main() 
{
    auto j;
    char s[64];
    srand(time(0));
    while(1) {
	auto n = rand()%64;
	for(j = 0; j < n; j++)
	    s[j] = rand()%26 + 'a';
	s[j] = 0;
	puts(s);
    }
}
