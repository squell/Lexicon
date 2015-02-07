#include <stdio.h>
#include <stdlib.h>

int main()
{
    int flag=0;
    srand(time(0));
    while(1) {
	int c = rand()%(26+flag);
	if(c >= 26) putchar('\n');
	else putchar(c+'a');
	flag = c<26;
    }
}
