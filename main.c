#include <stdio.h>

#include "firth.h"
#include "firth_float.h"

//
static struct FirthState *pFirth = NULL;

//
static myPrint(char *s)
{
	fputs(s, stdout);
}

//
void banner(FirthState *pFirth)
{
	pFirth->firth_print("Welcome to CFirth! Copyright 2022 by Mark Seminatore\n");
	pFirth->firth_print("See LICENSE file for usage rights and obligations.\n");
	pFirth->firth_print("Type 'bye' to quit.\n");
}

//
int main(int argc, char *argv[])
{
	// create a new Firth state object
	pFirth = fth_create_state();

	// use our output function
	fth_set_output_function(pFirth, myPrint);

	banner(pFirth);

#if FTH_INCLUDE_FLOAT == 1
	// load (optional) floating point libraries
	firth_register_float(pFirth);
#endif

	// REPL loop
	while (!pFirth->halted)
	{
		fth_update(pFirth);
	}

	// we're done, cleanup and quit
	fth_delete_state(pFirth);

	return 0;
}