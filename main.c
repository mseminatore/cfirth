#include <stdio.h>

#include "forth.h"

//
static struct ForthState *pForth = NULL;

//
static myPrint(char *s)
{
	fputs(s, stdout);
}

//
void banner(ForthState *pForth)
{
	pForth->forth_print("Welcome to CFirth! Copyright 2022 by Mark Seminatore\n");
	pForth->forth_print("See LICENSE file for usage rights and obligations.\n");
	pForth->forth_print("Type 'bye' to quit.\n");
}

//
int main(int argc, char *argv[])
{
	// create a new Forth state object
	pForth = fth_create_state();

	// use our output function
	fth_set_output_function(pForth, myPrint);

	banner(pForth);

	// REPL loop
	while (!pForth->halted)
	{
		fth_update(pForth);
	}

	// we're done, cleanup and quit
	fth_delete_state(pForth);

	return 0;
}