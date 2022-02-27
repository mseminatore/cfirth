#include <stdio.h>

#if defined(_WIN32) | defined(_WIN64)
#	include <Windows.h>
#endif

#include "firth.h"
#include "firth_float.h"

//
static struct FirthState *pFirth = NULL;

//
static void myPrint(char *s)
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
int setupConsole()
{
#if defined(_WIN32) | defined(_WIN64)
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		return GetLastError();
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
	{
		return GetLastError();
	}

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
	{
		return GetLastError();
	}
#endif

	return 0;
}

//
int main(int argc, char *argv[])
{
	setupConsole();

	// create a new Firth state object
	pFirth = fth_create_state();

	// use our output function
	fth_set_output_function(pFirth, myPrint);

	banner(pFirth);

	// if a file is given load it
	if (argc > 1)
	{
		char buf[FTH_MAX_PATH];
		sprintf(buf, "include %s", argv[1]);
		fth_parse_string(pFirth, buf);
	}

	// REPL loop
	while (!pFirth->halted)
	{
		fth_update(pFirth);
	}

	// we're done, cleanup and quit
	fth_delete_state(pFirth);

	return 0;
}