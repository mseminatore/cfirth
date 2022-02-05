#include <stdio.h>

#include "forth.h"

//
static struct ForthState *pForth;

//
static myPrint(char *s)
{
	fputs(s, stdout);
}

//
int main(int argc, char *argv[])
{
	// create a new Forth state object
	pForth = fth_create_state();

	// use our output function
	fth_set_output_function(pForth, myPrint);

	// REPL loop
	while (!pForth->halted)
	{
		fth_quit(pForth);
	}

	// we're done, cleanup and quit
	fth_delete_state(pForth);

	return 0;
}