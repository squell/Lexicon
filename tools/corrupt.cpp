#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdlib>
#include <ctime>

int main()
{
    char buf[1000];
    srand(time(0));
    while(gets(buf)) {
	if(*buf) buf[rand()%strlen(buf)] ^= 1;
	puts(buf);
    }
}
