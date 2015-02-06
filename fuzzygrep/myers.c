/*

Code implementing a simple fuzzy search based on

    'A fast bit-vector algorithm for approximate string matching based on dynamic programming'
    (Gene Myers, Journal of the ACM vol. 46 p1-13)

Copyright (c) 2012 Marc Schoolderman 

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.

*/

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#ifndef DEFAULT_DISTANCE
#define DEFAULT_DISTANCE 2
#endif

/* read the paper to understand the following three functions */

typedef unsigned long bits;

typedef struct {
    bits Peq[256];
    bits Pv, Mv;
    int Score;
    int Height;
} search_state[1];

/* precomputation: assumes table is already zeroed out */
void setup_search(search_state d, const char* str)
{
    int i;
    for(i=0; str[i]; ++i)
	d->Peq[str[i]&0xFF] |= 1uL << i;
    d->Height = i;
}

/* initialize the delta's */
void reset_search(search_state d)
{
    d->Score = d->Height;
    d->Pv = -1;
    d->Mv = 0;
}

int scan_char(search_state row, char c)
{
    bits Pv = row->Pv;
    bits Mv = row->Mv;
    bits Cf = (bits)1 << row->Height-1;
    bits Eq = row->Peq[c&0xFF];
    bits Xv = Eq | Mv;
    bits Xh = (Eq&Pv)+Pv ^ Pv | Eq;
    bits Ph = Mv | ~(Xh|Pv);
    bits Mh = Pv & Xh;
    if(Ph & Cf) 
	row->Score++;
    else if(Mh & Cf) 
	row->Score--;
    #ifdef FULLWORDS_ONLY
    Ph = Ph << 1 | 1;
    #else
    Ph <<= 1;
    #endif
    row->Pv = (Mh << 1) | ~(Xv|Ph);
    row->Mv = Ph & Xv;
    return row->Score;
}

/* Report a match to the user */
void emit_match(const char* str, int pos)
{
    printf("\t%s", str);
    printf("\t");
    while(pos-- > 0) {
	/* match whitespace exactly */
	char c = *str++;
	putchar(isspace(c)? c : ' ');
    }
    printf("^\n");
}

/* Process a file */
void scan_file(search_state state, int max_dist, FILE *input, const char* descr)
{
    char buf[0x1000];
    int line_count = 1;

    while(fgets(buf, sizeof buf, input)) {
	int prv_dist = max_dist+1;
	int emit = 0;  /* used to report only the locally optimal matches */
	int i = 0;
	char ch;

	reset_search(state);

	while((ch=buf[i]) != '\n') {
	    int dist = scan_char(state, ch);
	    if(ch == '\0') {
		printf("%s:line %d; long input; skipping\n", descr, line_count);
		return;
	    }
	    #ifdef FULLWORDS_ONLY
	    if(!isalnum(ch)) {
		if(prv_dist <= max_dist && emit) {
		    printf("%s:line %d, distance %d\n", descr, line_count, prv_dist);
		    emit_match(buf, i-1);
		}
		reset_search(state);
		emit = 0;
	    #else
	    if(prv_dist < dist && prv_dist <= max_dist && emit) {
		/* found a good match at previous position */
		printf("%s:line %d, distance %d\n", descr, line_count, prv_dist);
		emit_match(buf, i-1);
		emit = 0;
	    #endif
	    } else if(dist < prv_dist) {
		emit = 1;
	    }
	    prv_dist = dist;
	    i++;
	}

	/* check if end of line should match */
	if(prv_dist <= max_dist && emit) {
	    printf("%s:line %d, distance %d\n", descr, line_count, prv_dist);
	    emit_match(buf, i-1);
	}

	line_count++;
    }
}


int main(int argc, char *argv[])
{
    static search_state state;
    int max_dist;

    /* only rudimentary sanity checks */
    if(argc < 2) {
	puts("usage: fuzz <string> [distance] [filespec]");
	return 1;
    }

    setup_search(state, argv[1]);

    /* if the second argument seems numeric, treat as custom distance */
    if(argc > 2 && isdigit(argv[2][0])) {
	max_dist = atoi(argv[2]);
	argv++;
    } else
	max_dist = DEFAULT_DISTANCE;

    /* check if pattern fits in the machine word size */
    if(state->Height > sizeof(bits)*CHAR_BIT) {
	printf("Pattern too long; maximum length is %d\n", (int)sizeof(bits)*CHAR_BIT);
	return 1;
    }

    /* there is no point in setting the distance higher */
    if(max_dist >= state->Height) 
	printf("Reducing maximum edit distance to %d\n", max_dist = state->Height-1);

    /* process the arguments, or stdin if none */
    if(!argv[2])
	scan_file(state, max_dist, stdin, "<stdin>");
    else do {
	FILE *f = fopen(argv[2], "r");
	if(!f) {
	    printf("Could not open file: %s; skipping\n", argv[2]);
	    continue;
	}
	scan_file(state, max_dist, f, argv[2]);
	fclose(f);
    } while((++argv)[2]);

    return 0;
}

