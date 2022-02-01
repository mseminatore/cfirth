#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "forth.h"

//
bool true = ~0;
bool false = 0;

static DictionaryEntry *fth_tick_internal(ForthState *pForth, char *word);

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
static int isInteger(const char *s)
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

// given an xt get the 
static int* fth_body(ForthState *pforth, DictionaryEntry *xt)
{
	return (int*)(xt->name + xt->name_len + 1);
}

// push a value onto the return stack
static int fth_r_push(ForthState *pForth, int val)
{
	// check for return stack overflow
	if ((pForth->RP - pForth->return_stack) + 1 > FTH_RETURN_STACK_SIZE)
	{
		pForth->forth_print("Return Stack overflow!\n");
		return FTH_FALSE;
	}

	*pForth->RP++ = val;

	return FTH_TRUE;
}

// pop a value off the return stack
static int fth_r_pop(ForthState *pForth)
{
	// check for stack underflow
	if (pForth->RP <= pForth->return_stack)
	{
		pForth->forth_print("Return Stack underflow!\n");
		return FTH_FALSE;
	}

	pForth->RP--;
	return *pForth->RP;
}

// push the top of return stack onto the stack
static int fth_r_fetch(ForthState *pForth)
{
	// check for stack underflow
	if (pForth->RP <= pForth->return_stack)
	{
		pForth->forth_print("Return Stack underflow!\n");
		return FTH_FALSE;
	}

	return fth_push(pForth, *(pForth->RP - 1));
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

// create a new empty dictionary entry withe name given by word
static int fth_make_dict_entry(ForthState *pForth, char *word)
{
	// see if the word already exists and if so warn about it
	DictionaryEntry *pEntry = (DictionaryEntry*)fth_tick_internal(pForth, word);
	if (pEntry)
	{
		pForth->forth_print("Note: redefining an existing word!\n");
	}

	DictionaryEntry *pNewHead = (DictionaryEntry *)pForth->CP;
	pNewHead->code_pointer = NULL;
	pNewHead->next = pForth->head;

	pNewHead->flags.immediate = 0;
	pNewHead->flags.hidden = 0;
	pNewHead->flags.colon_word = 0;
	pNewHead->flags.compile_only = 0;
	pNewHead->flags.xt_on_stack = 0;
	pNewHead->flags.unused = 0;

	pNewHead->name_len = (BYTE)(strlen(word) + 1);	// +1 to allow for null terminator
	strcpy(pNewHead->name, word);

	pForth->head = pNewHead;
	pForth->CP += sizeof(DictionaryEntry) + pForth->head->name_len + 1;

	return FTH_TRUE;
}

//
static int fth_write_to_cp(ForthState *pForth, int val)
{
	*((int*)pForth->CP) = val;
	pForth->CP += sizeof(int);		// advance dictionary pointer

	return FTH_TRUE;
}

// run-time implementation of CONSTANT
static int fth_const_imp(ForthState *pForth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pForth);

	int val = *fth_body(pForth, pDict);
	fth_push(pForth, val);

	return FTH_TRUE;
}

// run-time implementation of VARIABLE
static int fth_var_imp(ForthState *pForth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pForth);

	int addr = (int)fth_body(pForth, pDict);
	fth_push(pForth, addr);

	return FTH_TRUE;
}

// create a CONSTANT word
static int fth_const(ForthState *pForth)
{
	// create a new dictionary entry for this constant
	fth_create(pForth);

	// the stack at this point contains | const_val xt |
	// so we pop the xt
//	fth_pop(pForth);
	int n = fth_pop(pForth);

	pForth->head->code_pointer = fth_const_imp;
	pForth->head->flags.xt_on_stack = 1;

	// store constant value in dictionary
	fth_write_to_cp(pForth, n);

	return FTH_TRUE;
}

// create a VARIABLE word
static int fth_var(ForthState *pForth)
{
	// create a new dictionary entry for this variable
	fth_create(pForth);

	// the stack at this point contains | xt |
//	fth_pop(pForth);

	pForth->head->code_pointer = fth_var_imp;
	pForth->head->flags.xt_on_stack = 1;

	// store constant value in dictionary
	fth_write_to_cp(pForth, FTH_UNINITIALIZED);

	return FTH_TRUE;
}

// interpret a NUMBER
static int fth_number(ForthState *pForth)
{
	int n = atoi(pForth->TIB);
	return fth_push(pForth, n);
}

// push a literal value onto the stack
static int fth_literal(ForthState *pForth)
{
	// fetch the next address and use it to get the literal value
	int val = *pForth->IP++;
	
	// push literal value onto the stack
	fth_push(pForth, val);
	return FTH_TRUE;
}

//
static DictionaryEntry *fth_tick_internal(ForthState *pForth, char *word)
{
	DictionaryEntry *pDict = (DictionaryEntry*)pForth->head;

	for (; pDict; pDict = pDict->next)
	{
		if (!_stricmp(pDict->name, word))
		{
			// return the dictionary entry
			return pDict;
		}
	}

	return NULL;
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
		if (!_stricmp(pDict->name, pForth->TIB))
		{
			// push xt for the word onto the stack
			return fth_push(pForth, (int)pDict);
		}
	}

	//	fth_push(pForth, 0);
	return FTH_FALSE;
}

// execute a colon word
static int fth_address_interpreter(ForthState *pForth)
{
	// get the xt from the stack
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pForth);

	// use the xt to set the IP
	pForth->IP = (int*)(pDict->name + pDict->name_len + 1);

	while (*pForth->IP)
	{
		DictionaryEntry *pDict = (DictionaryEntry*)*pForth->IP++;
		ForthFunc f = (ForthFunc)pDict->code_pointer;

		// if the next word is a colon word
		if (pDict->flags.colon_word)
		{
			// push IP on return stack
			fth_r_push(pForth, (int)pForth->IP);
		}
		else
			f(pForth);
	}

	// pop IP off return stack
	pForth->IP = (int*)fth_r_pop(pForth);

	return FTH_TRUE;
}

// start compiling a new word
static int fth_colon(ForthState *pForth)
{
	// create a new dictionary entry
	fth_create(pForth);

	// 
	pForth->head->code_pointer = fth_address_interpreter;
	pForth->head->flags.colon_word = 1;

	pForth->compiling = true;
	return FTH_TRUE;
}

// start defining a new WORD
static int fth_semicolon(ForthState *pForth)
{
	// mark the EXIT of this word
	fth_write_to_cp(pForth, 0);

	pForth->compiling = false;
	return FTH_TRUE;
}

// compile the words in a new colon definition
static int fth_compile(ForthState *pForth)
{
	// get the xt of the word to compile
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pForth);

	// if it is an immediate word execute it, otherwise thread it
	if (pDict->flags.immediate)
	{
		ForthFunc f = (ForthFunc)pDict->code_pointer;
		return f(pForth);
	}
	else
		fth_write_to_cp(pForth, (int)pDict);

	return FTH_TRUE;
}

// execute the xt at SP
static int fth_execute(ForthState *pForth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_peek(pForth);

	// do not execute compile only words
	if (pDict->flags.compile_only)
	{
		pForth->forth_print(" action is not a function\n");
		return FTH_FALSE;
	}

	// leave the xt on the stack only if it is a colon word or a defining word
	if ( !pDict->flags.colon_word && !pDict->flags.xt_on_stack)
	{
		fth_pop(pForth);
	}

	ForthFunc f = (ForthFunc)pDict->code_pointer;

	int r = f(pForth);

	return r;
}

// create a new empty dictionary entry
static int fth_create(ForthState *pForth)
{
	// get the next word in input stream
	fth_push(pForth, ' ');
	fth_word(pForth);

	return fth_make_dict_entry(pForth, pForth->TIB);
}

// make the last word created immediate
static int fth_immediate(ForthState *pForth)
{
	pForth->head->flags.immediate = 1;
	return FTH_TRUE;
}

// make the last word created hidden
static int fth_hide(ForthState *pForth)
{
	pForth->head->flags.hidden = 1;
	return FTH_TRUE;
}

// computes the dictionary space needed for N cells
static int fth_cells(ForthState *pForth)
{
	int n = fth_pop(pForth);
	return fth_push(pForth, n * sizeof(int));
}

// reserve N bytes in the dictionary
static int fth_allot(ForthState *pForth)
{
	int n = fth_pop(pForth);
	pForth->CP += n;

	return FTH_TRUE;
}

//
static int fth_stop_compile(ForthState *pForth)
{
	pForth->compiling = 0;
	return FTH_TRUE;
}

//
static int fth_start_compile(ForthState *pForth)
{
	pForth->compiling = 1;
	return FTH_TRUE;
}

//
static int fth_dots(ForthState *pForth)
{
	// make a copy of the stack
	ForthNumber *s = pForth->SP;

	pForth->forth_print("Top -> [ ");
	while (s-- != pForth->stack)
	{
		//if (hexmode)
		//	firth_printf("0x%0X ", s.top());
		//else
			forth_printf(pForth, "%d ", *s);
	}
	pForth->forth_print("]\n");

	return FTH_TRUE;
}

// push the current stack size onto the stack
static int fth_depth(ForthState *pForth)
{
	int depth = pForth->SP - pForth->stack;
	fth_push(pForth, depth);
	return FTH_TRUE;
}

// execute a marker to reset dictionary state
static int fth_marker_imp(ForthState *pForth)
{
	DictionaryEntry *xt = (DictionaryEntry*)fth_pop(pForth);

	// reset dictionary head
	pForth->head = xt->next;

	// reset CP
	pForth->CP = (BYTE*)xt;
	return FTH_TRUE;
}

// create a state marker in the dictionary
static int fth_marker(ForthState *pForth)
{
	// create a new dictionary entry for this marker
	fth_create(pForth);

	pForth->head->code_pointer = fth_marker_imp;
	pForth->head->flags.xt_on_stack = 1;

	return FTH_TRUE;
}

//
//
// Note: Since word lookup is O(n) on dictionary size
// try to keep common words at the bottom for speed
static const ForthWordSet basic_lib[] =
{
	{ "EXECUTE", fth_execute },
	{ "MARKER", fth_marker },
	{ "INTERPRET", fth_interpret },
	{ "NUMBER", fth_number },
	{ "'", fth_tick },
	{ "WORD", fth_word },

	{ "COMPILE", fth_compile },
	{ "IMMEDIATE", fth_immediate },
	{ "HIDE", fth_hide },
	{ "[", fth_stop_compile },
	{ "]", fth_start_compile },

	// defining and related words
	{ "CREATE", fth_create },
	{ "CONSTANT", fth_const },
	{ "VARIABLE", fth_var },
	{ ":", fth_colon },
	{ ";", fth_semicolon },

	{ "DEPTH", fth_depth },
	{ ".S", fth_dots },

	{ "KEY", fth_key },
	{ "CELLS", fth_cells },
	{ "ALLOT", fth_allot },

	{ NULL, NULL }
};

//========================
// Public Forth functions
//========================

// allows embedders to set their own output function
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

	pForth->return_stack = malloc(FTH_RETURN_STACK_SIZE);
	if (!pForth->return_stack)
	{
		free(pForth->dictionary_base);
		free(pForth->stack);
		free(pForth);
		return NULL;
	}

	// setup return stack
	pForth->SP = pForth->stack;
	pForth->RP = pForth->return_stack;

	// setup our internal state
	pForth->CP = pForth->dictionary_base;			// setup dictionary pointer
	pForth->IP = NULL;
	pForth->head = NULL;							// dictionary is empty at this point
	pForth->forth_print = forth_default_output;		// setup default print routine
	pForth->halted = 0;
	pForth->IN = pForth->TIB;						// TODO - not yet used
	pForth->compiling = false;						// start in interpreter mode

	// register built-in words
	fth_register_wordset(pForth, basic_lib);
	fth_register_core_wordset(pForth);

	// setup immediate words
	fth_make_immediate(pForth, ";");
	fth_make_immediate(pForth, "[");

	// setup compile only words
	fth_make_compile_only(pForth, ";");

	// setup defining words

	// setup hidden words

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
	if ((pForth->SP - pForth->stack) + 1 > FTH_STACK_SIZE)
	{
		pForth->forth_print("Stack overflow!\n");
		return FTH_FALSE;
	}

	*pForth->SP++ = val;

	return FTH_TRUE;
}

// pop a value off the stack
int fth_pop(ForthState *pForth)
{
	// check for stack underflow
	if (pForth->SP <= pForth->stack)
	{
		pForth->forth_print("Stack underflow!\n");
		return FTH_FALSE;
	}

	pForth->SP--;
	return *pForth->SP;
}

// return the TOS without removing it
int fth_peek(ForthState *pForth)
{
	if (pForth->SP == pForth->stack)
	{
		pForth->forth_print("Stack is empty!\n");
		return FTH_FALSE;
	}

	return *(pForth->SP-1);
}

// outer interpreter
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
	else if (isInteger(pForth->TIB))
		fth_number(pForth);
	else
		forth_printf(pForth, "%s ?\n", pForth->TIB);

	pForth->forth_print(" ok\n");

	return FTH_TRUE;
}

//
int fth_make_hidden(ForthState* pForth, char *word)
{
	DictionaryEntry *pEntry = fth_tick_internal(pForth, word);
	if (!pEntry)
	{
		pForth->forth_print("Word not found.");
		return FTH_FALSE;
	}

	pEntry->flags.hidden = 1;
	return FTH_TRUE;
}

//
int fth_make_immediate(ForthState* pForth, char *word)
{
	DictionaryEntry *pEntry = fth_tick_internal(pForth, word);
	if (!pEntry)
	{
		pForth->forth_print("Word not found.");
		return FTH_FALSE;
	}

	pEntry->flags.immediate = 1;
	return FTH_TRUE;
}

//
int fth_make_compile_only(ForthState* pForth, char *word)
{
	DictionaryEntry *pEntry = fth_tick_internal(pForth, word);
	if (!pEntry)
	{
		pForth->forth_print("Word not found.");
		return FTH_FALSE;
	}

	pEntry->flags.compile_only = 1;
	return FTH_TRUE;
}

//
int fth_make_xt_required(ForthState* pForth, char *word)
{
	DictionaryEntry *pEntry = fth_tick_internal(pForth, word);
	if (!pEntry)
	{
		pForth->forth_print("Word not found.");
		return FTH_FALSE;
	}

	pEntry->flags.xt_on_stack = 1;
	return FTH_TRUE;
}

//
//int fth_quit(ForthState *pForth)
//{
//	pForth->forth_print("forth>");
//
//	pForth->forth_print(" ok\n");
//}
