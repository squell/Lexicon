#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(0);
    string buf;
    srand(time(0));
    while(getline(cin, buf)) {
	if(!buf.empty()) buf[rand()%buf.length()] ^= 1;
	cout.rdbuf()->sputn(buf.data(), buf.length());
	cout.rdbuf()->sputc('\n');
    }
}
