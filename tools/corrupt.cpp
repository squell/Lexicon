#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

int main()
{
    string buf;
    srand(time(0));
    while(getline(cin, buf)) {
	if(!buf.empty()) buf[rand()%buf.length()] ^= 1;
	cout << buf << endl;
    }
}
