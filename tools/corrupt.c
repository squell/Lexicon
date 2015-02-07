main()
{
    char buf[1000];
    srand(time((void*)0));
    while(gets(buf)) {
	if(*buf) buf[rand()%strlen(buf)] ^= 1;
	puts(buf);
    }
}
