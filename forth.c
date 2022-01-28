#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "forth.h"

//
static void forth_default_output(char *s)
{
	puts(s);
}

// Forth formatted output function
void forth_printf(ForthState *pForth, char *format, ...)
{
	char buf[FTH_MAX_PRINTF_SIZE];
	va_list valist;

	va_start(valist, format);
	vsprintf(buf, format, valist);
	va_end(valist);

	pForth->forth_print(buf);
}

//
static int isInt(const char *s)
{
	if (*s != '-' && *s != '+' && !isdigit(*s))
		return FTH_FALSE;

	s++;

	for (; *s; s++)
	{
		if (!isdigit(*s))
			return FTH_FALSE;
	}

	return FTH_TRUE;
}

// execute the xt at TOS
static int fth_execute(ForthState *pForth)
{
	ForthFunc f = (ForthFunc)fth_pop(pForth);
	return f(pForth);
}

//
static int fth_number(ForthState *pForth)
{
	int n = atoi(pForth->TIB);

	return fth_push(pForth, n);
}

// implements TICK word
static int fth_tick(ForthState *pForth)
{
	// get a WORD from input
	fth_push(pForth, ' ');
	fth_word(pForth);

	DictionaryEntry *pDict = (DictionaryEntry*)pForth->head;

	for (; pDict; pDict = pDict->next)
	{
		if (!stricmp(pDict->name, pForth->TIB))
		{
			// push xt for the word onto the stack
			pForth->current_word = pDict;
			return fth_push(pForth, (int)pDict->code_pointer);
		}
	}

//	fth_push(pForth, 0);
	return FTH_FALSE;
}

//
static int fth_key(ForthState *pForth)
{
	return fth_push(pForth, getc(stdin));
}

// read in a line of text
static int fth_accept(ForthState *pForth)
{
	int c;
	
	char *p = pForth->TIB;

	// read a line into the TIB
	while ((c = getc(stdin)) != '\n')
	{
		*p++ = c;
	}

	// set input pointer to start of buffer
	pForth->IN = pForth->TIB;
}

// implements WORD word
static int fth_word(ForthState *pForth)
{
	int c;

	int delim = fth_pop(pForth);

	// skip leading spaces
	while ((c = getc(stdin)) == delim);

	// read until 
	char *p = pForth->TIB;
	while (c != delim && c != '\n' && c != EOF)
	{
		*p++ = c;
		c = getc(stdin);
	}

	// make word in TIB asciiz
	*p = '\0';

	return FTH_TRUE;
}

//
static int fth_make_dict_entry(ForthState *pForth, char *word)
{
	DictionaryEntry *pNewHead = (DictionaryEntry *)pForth->CP;
	pNewHead->code_pointer = NULL;
	pNewHead->next = pForth->head;
	pNewHead->name_len = (BYTE)(strlen(word) + 1);	// +1 to allow for null terminator
	strcpy(pNewHead->name, word);

	pForth->head = pNewHead;
	pForth->CP += sizeof(DictionaryEntry) + pForth->head->name_len + 1;

	return FTH_TRUE;
}

// create a new dictionary entry
static int fth_create(ForthState *pForth)
{
	// get the next word in input stream
	fth_push(pForth, ' ');
	fth_word(pForth);

	return fth_make_dict_entry(pForth, pForth->TIB);
}

//
//static int fth_setxt(ForthState *pForth, ForthFunc f)
//{
//	pForth->head->code_pointer = f;
//	return FTH_TRUE;
//}

//
static int fth_const_imp(ForthState *pForth)
{
	char *s = pForth->current_word->name + pForth->current_word->name_len + 1;

	int val = *(int*)s;
	fth_push(pForth, val);

	return FTH_TRUE;
}

//
static int fth_const(ForthState *pForth)
{
	fth_create(pForth);
	int n = fth_pop(pForth);

	pForth->head->code_pointer = fth_const_imp;

	// store constant value in dictionary
	*((int*)pForth->CP) = n;
	pForth->CP += sizeof(int);		// advance dictionary pointer

	return FTH_TRUE;
}

//
static int fth_var_imp(ForthState *pForth)
{
	int val = (int)pForth->current_word->name + pForth->current_word->name_len + 1;
	fth_push(pForth, val);

	return FTH_TRUE;
}

//
static int fth_var(ForthState *pForth)
{
	fth_create(pForth);

	pForth->head->code_pointer = fth_var_imp;

	// store constant value in dictionary
	int *pInt = (int*)pForth->CP;
	*(pInt) = 0;					// vars initialize to 0
	pForth->CP += sizeof(int);		// advance dictionary pointer

	return FTH_TRUE;
}

//
static int fth_colon(ForthState *pForth)
{
	pForth->compiling = true;
}

//
static int fth_semicolon(ForthState *pForth)
{
}

//
static int fth_compile(ForthState *pForth)
{
}

//
static const ForthWordSet basic_lib[] =
{
	{ "EXECUTE", fth_execute },
	{ "INTERPRET", fth_interpret },
	{ "NUMBER", fth_number },
	{ "'", fth_tick },
	{ "WORD", fth_word },
	{ "CREATE", fth_create },
	{ "CONSTANT", fth_const },
	{ "VARIABLE", fth_var },
	{ "KEY", fth_key },
	
	{ ":", fth_colon },
	{ ";", fth_semicolon },
	{ "COMPILE", fth_compile },

	{ NULL, NULL }
};

//==================
// Public functions
//==================

//
int fth_set_output_function(ForthState *pForth, ForthOutputFunc f)
{
	pForth->forth_print = f;
	return FTH_TRUE;
}

// create a new forth state
ForthState *fth_create_state()
{
	ForthState *pForth = malloc(sizeof(ForthState));
	if (!pForth)
		return NULL;

	pForth->dictionary_base = malloc(FTH_ENV_SIZE);
	if (!pForth->dictionary_base)
	{
		free(pForth);
		return NULL;
	}

	pForth->stack = malloc(FTH_STACK_SIZE);
	if (!pForth->stack)
	{
		free(pForth->dictionary_base);
		free(pForth);
		return NULL;
	}

	pForth->CP = pForth->dictionary_base;
	pForth->IP = NULL;
	pForth->head = NULL;
	pForth->forth_print = forth_default_output;
	pForth->TOS = pForth->stack;
	pForth->halted = 0;
	pForth->IN = pForth->TIB;
	pForth->current_word = NULL;
	pForth->compiling = false;

	// register built-in words
	fth_register_wordset(pForth, basic_lib);
	fth_register_core_wordset(pForth);

	return pForth;
}

// cleanup and free forth state
int fth_delete_state(ForthState *pForth)
{
	free(pForth->dictionary_base);
	free(pForth->stack);
	free(pForth);

	return FTH_TRUE;
}

//
int fth_register_wordset(ForthState *pForth, const ForthWordSet words[])
{
	for (int i = 0; words[i].wordName && words[i].func; i++)
	{
		fth_make_dict_entry(pForth, words[i].wordName);
		pForth->head->code_pointer = words[i].func;
	}

	return FTH_TRUE;
}

// push a value on the stack
int fth_push(ForthState *pForth, int val)
{
	// check for stack overflow
	if ((pForth->TOS - pForth->stack) + 1 > FTH_STACK_SIZE)
	{
		pForth->forth_print("Stack overflow!\n");
		return FTH_FALSE;
	}

	*pForth->TOS++ = val;

	return FTH_TRUE;
}

// pop a value off the stack
int fth_pop(ForthState *pForth)
{
	// check for stack underflow
	if (pForth->TOS <= pForth->stack)
	{
		pForth->forth_print("Stack underflow!\n");
		return FTH_FALSE;
	}

	pForth->TOS--;
	return *pForth->TOS;
}

//
int fth_interpret(ForthState *pForth)
{
	pForth->forth_print("forth>");

	// if WORD exists, execute it
	if (fth_tick(pForth))
	{
		if (pForth->compiling)
			fth_compile(pForth);
		else
			fth_execute(pForth);
	}
	else if (isInt(pForth->TIB))
		fth_number(pForth);
	else
		forth_printf(pForth, "%s ?\n", pForth->TIB);

	pForth->forth_print(" ok\n");

	return FTH_TRUE;
}

//
int fth_quit(ForthState *pForth)
{
	pForth->forth_print("forth>");

	pForth->forth_print(" ok\n");
}
