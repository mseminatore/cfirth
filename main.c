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

// examples of calling Firth from C
static void callFirth(FirthState *pFirth)
{
	// exec_word is a set of convenience functions to push 
	// 0, 1, 2, or 3 parameters on stack and execute a word
	fth_exec_word(pFirth, "words");

	fth_exec_word2(pFirth, "+", 1, 2);

	// parse, compile and execute a linen of text
	fth_parse_string(pFirth, ": star 42 emit ;");
}

//
void banner(FirthState *pFirth)
{
	pFirth->firth_print("Welcome to C-Firth! Copyright 2022 by Mark Seminatore\n");
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

	// REPL loop
	while (!pFirth->halted)
	{
		fth_update(pFirth);
	}

	// we're done, cleanup and quit
	fth_delete_state(pFirth);

	return 0;
}