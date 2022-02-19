#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#include "firth.h"

#if FTH_INCLUDE_FLOAT == 1
#	include "firth_float.h"
#endif

//
bool true = ~0;
bool false = 0;

static DictionaryEntry *fth_tick_internal(FirthState *pFirth, const char *word);

#if 0
#	define LOG(s)	puts(s);
#else
#	define LOG(s)
#endif

//
static void forth_default_output(char *s)
{
	puts(s);
}

// Firth formatted output function
void firth_printf(FirthState *pFirth, char *format, ...)
{
	char buf[FTH_MAX_PRINTF_SIZE];
	va_list valist;

	va_start(valist, format);
	vsprintf(buf, format, valist);
	va_end(valist);

	pFirth->firth_print(buf);
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

// given an xt get the data pointer
static int* fth_body_internal(DictionaryEntry *xt)
{
	return (int*)(xt->name + xt->name_len + 1);
}

// implements >BODY ( addr -- d )
static int fth_body(FirthState *pFirth)
{
	DictionaryEntry *xt = (DictionaryEntry *)fth_pop(pFirth);
	return fth_push(pFirth, (FirthNumber)(int*)(xt->name + xt->name_len + 1));
}

// push a value onto the return stack
static int fth_r_push(FirthState *pFirth, FirthNumber val)
{
	// check for return stack overflow
	if ((pFirth->RP - pFirth->return_stack) + 1 > FTH_RETURN_STACK_SIZE)
	{
		pFirth->firth_print("Return Stack overflow!\n");
		return FTH_FALSE;
	}

	*pFirth->RP++ = val;

	FirthNumber depth = pFirth->RP - pFirth->return_stack;
	if (depth > pFirth->maxr)
		pFirth->maxr = depth;

	return FTH_TRUE;
}

// pop a value off the return stack
static FirthNumber fth_r_pop(FirthState *pFirth)
{
	// check for stack underflow
	if (pFirth->RP <= pFirth->return_stack)
	{
		pFirth->firth_print("Return Stack underflow!\n");
		return FTH_FALSE;
	}

	pFirth->RP--;
	return *pFirth->RP;
}

// push the top of return stack onto the stack
static FirthNumber fth_r_fetch(FirthState *pFirth)
{
	// check for stack underflow
	if (pFirth->RP <= pFirth->return_stack)
	{
		pFirth->firth_print("Return Stack underflow!\n");
		return FTH_FALSE;
	}

	return fth_push(pFirth, *(pFirth->RP - 1));
}

// implements KEY - get a new key from the terminal
static int fth_key(FirthState *pFirth)
{
	return fth_push(pFirth, getc(stdin));
}

// implements ACCEPT - read in a line of text
// ( addr limit -- size )
static int fth_accept(FirthState *pFirth)
{
	int c;
	FirthNumber count = 0;

	LOG("ACCEPT");

	FirthNumber limit = fth_pop(pFirth);
	FirthNumber addr = fth_pop(pFirth);
	
	char *p = (char*)addr;

	// read at most limit characters from the input and store at addr
	while ((c = getc(pFirth->BLK)) != '\n' && count < limit)
	{
		*p++ = c;
		count++;
	}

	// make sure input string is null terminated
	memset(p, 0, FTH_MAX_WORD_NAME - (p - pFirth->TIB));

	pFirth->IN = pFirth->TIB;

	fth_push(pFirth, count);

	return FTH_TRUE;
}

//
static int fth_fill_tib(FirthState *pFirth)
{
	// fill input bufer
	fth_push(pFirth, (FirthNumber)pFirth->TIB);
	fth_push(pFirth, sizeof(pFirth->TIB) - 1);
	fth_accept(pFirth);
	pFirth->tib_len = fth_pop(pFirth);

	return FTH_TRUE;
}

// implements WORD - read in a new word with the given delimiter
// ( delim -- addr )
static int fth_word(FirthState *pFirth)
{
	int c;

	LOG("WORD");

	int delim = fth_pop(pFirth);

	// skip any leading delimiter chars
	while ((c = *pFirth->IN) == delim)
		pFirth->IN++;

	// start with the current input pointer
	char *p = pFirth->IN;

	// at this point c contains a non delimiter character

	// read until delimiter, EOL, EOF or buffer end
	while ( (p - pFirth->TIB < pFirth->tib_len) && c != 0 && c != delim && c != '\n' && c != EOF )
	{
		p++;
		c = *p;
	}

	if (c == EOF)
	{
		if (pFirth->BLK != stdin)
		{
			// close previous file
			fclose(pFirth->BLK);

			// set input to terminal
			pFirth->BLK = stdin;
		}
	}

	// make word in TIB asciiz
	*p = '\0';

	// put addr of word on stack
	fth_push(pFirth, (FirthNumber)pFirth->IN);
	
	// advance IN pointer to beyond the word
	pFirth->IN = p + 1;

	return FTH_TRUE;
}

// create a new empty dictionary entry withe name given by word
static int fth_make_dict_entry(FirthState *pFirth, const char *word)
{
	// see if the word already exists and if so warn about it
	DictionaryEntry *pEntry = (DictionaryEntry*)fth_tick_internal(pFirth, word);
	if (pEntry)
	{
		firth_printf(pFirth, "Note: redefining an existing word (%s).\n", pEntry->name);
	}

	DictionaryEntry *pNewHead = (DictionaryEntry *)pFirth->CP;
	pNewHead->code_pointer = NULL;
	pNewHead->next = pFirth->head;

	pNewHead->flags.immediate = 0;
	pNewHead->flags.hidden = 0;
	pNewHead->flags.colon_word = 0;
	pNewHead->flags.compile_only = 0;
	pNewHead->flags.xt_on_stack = 0;
	pNewHead->flags.constant = 0;
	pNewHead->flags.variable = 0;
	pNewHead->flags.unused = 0;

	pNewHead->name_len = (BYTE)(strlen(word) + 1);	// +1 to allow for null terminator
	strcpy(pNewHead->name, word);

	pFirth->head = pNewHead;
	pFirth->CP += sizeof(DictionaryEntry) + pFirth->head->name_len + 1;

	return FTH_TRUE;
}

//
static int fth_write_to_cp(FirthState *pFirth, FirthNumber val)
{
	*((FirthNumber*)pFirth->CP) = val;
	pFirth->CP += sizeof(FirthNumber);		// advance dictionary pointer

	return FTH_TRUE;
}

#if FTH_INCLUDE_FLOAT == 1
// run-time implementation of FCONSTANT
static int fth_fconst_imp(FirthState *pFirth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pFirth);

	FirthFloat val = *(FirthFloat*)fth_body_internal(pDict);
	fth_pushf(pFirth, val);

	return FTH_TRUE;
}

// run-time implementation of FVARIABLE
static int fth_fvar_imp(FirthState *pFirth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pFirth);

	int addr = (int)fth_body_internal(pDict);
	fth_push(pFirth, addr);

	return FTH_TRUE;
}

// run-time implementation of user FVARIABLE
static int fth_user_fvar_imp(FirthState *pFirth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pFirth);

	FirthFloat **ppVar = (FirthFloat**)fth_body_internal(pDict);
	fth_pushf(pFirth, (FirthFloat)**ppVar);

	return FTH_TRUE;
}

// create a FCONSTANT word
static int fth_fconst(FirthState *pFirth)
{
	// create a new dictionary entry for this constant
	fth_create(pFirth);

	// the stack at this point contains | const_val |
	FirthFloat n = fth_popf(pFirth);

	pFirth->head->code_pointer = fth_fconst_imp;
	pFirth->head->flags.xt_on_stack = 1;
	pFirth->head->flags.constant = 1;

	// store constant value in dictionary
	fth_write_to_cp(pFirth, *(int*)&n);

	return FTH_TRUE;
}

// create a FVARIABLE word
static int fth_fvar(FirthState *pFirth)
{
	// create a new dictionary entry for this variable
	fth_create(pFirth);

	pFirth->head->code_pointer = fth_fvar_imp;
	pFirth->head->flags.xt_on_stack = 1;
	pFirth->head->flags.variable = 1;

	// store value in dictionary
	fth_write_to_cp(pFirth, FTH_UNINITIALIZED);

	return FTH_TRUE;
}

// push a float literal value onto the stack
static int fth_fliteral(FirthState *pFirth)
{
	// fetch the next address and use it to get the literal value
	FirthFloat val = *(FirthFloat*)pFirth->IP++;

	// push literal value onto the stack
	fth_pushf(pFirth, val);
	return FTH_TRUE;
}

#endif

// run-time implementation of CONSTANT
static int fth_const_imp(FirthState *pFirth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pFirth);

	int val = *fth_body_internal(pDict);
	fth_push(pFirth, val);

	return FTH_TRUE;
}

// run-time implementation of VARIABLE
static int fth_var_imp(FirthState *pFirth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pFirth);

	FirthNumber addr = (int)fth_body_internal(pDict);
	fth_push(pFirth, addr);

	return FTH_TRUE;
}

//
static int fth_user_var_imp(FirthState *pFirth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pFirth);

	FirthNumber **ppVar = (FirthNumber**)fth_body_internal(pDict);
	fth_push(pFirth, (FirthNumber)*ppVar);

	return FTH_TRUE;
}

// create a CONSTANT word
static int fth_const(FirthState *pFirth)
{
	// create a new dictionary entry for this constant
	fth_create(pFirth);

	// the stack at this point contains | const_val |
	FirthNumber n = fth_pop(pFirth);

	pFirth->head->code_pointer = fth_const_imp;
	pFirth->head->flags.xt_on_stack = 1;
	pFirth->head->flags.constant = 1;

	// store constant value in dictionary
	fth_write_to_cp(pFirth, n);

	return FTH_TRUE;
}

// create a VARIABLE word
static int fth_var(FirthState *pFirth)
{
	// create a new dictionary entry for this variable
	fth_create(pFirth);

	pFirth->head->code_pointer = fth_var_imp;
	pFirth->head->flags.xt_on_stack = 1;
	pFirth->head->flags.variable = 1;

	// store value in dictionary
	fth_write_to_cp(pFirth, FTH_UNINITIALIZED);

	return FTH_TRUE;
}

// implements NUMBER - look at input and if a number push it onto the stack
// Note: if not a number leaves input addr on stack
static int fth_number(FirthState *pFirth)
{
	char *input = (char*)fth_peek(pFirth);
	if (isInteger(input))
	{
		FirthNumber n = atoi(input);
		fth_pop(pFirth);

		return fth_push(pFirth, n);
	}

#if FTH_INCLUDE_FLOAT == 1
	if (isdigit(input[0]) && (strchr(input, '.') || strchr(input, 'e')))
	{
		FirthFloat num = (FirthFloat)atof(input);
		fth_pop(pFirth);

		return fth_pushf(pFirth, num);
	}
#endif

	return FTH_FALSE;
}

// push a literal value onto the stack
static int fth_literal(FirthState *pFirth)
{
	// fetch the next address and use it to get the literal value
	int val = *pFirth->IP++;
	
	// push literal value onto the stack
	fth_push(pFirth, val);
	return FTH_TRUE;
}

//
static DictionaryEntry *fth_tick_internal(FirthState *pFirth, const char *word)
{
	DictionaryEntry *pDict = (DictionaryEntry*)pFirth->head;

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

//
static int fth_interpreter_tick(FirthState *pFirth)
{
	// get a WORD from input
	fth_push(pFirth, ' ');
	fth_word(pFirth);

	char *word = (char*)fth_peek(pFirth);

	// if buffer is empty return
	if (0 == *word)
	{
		fth_push(pFirth, FTH_FALSE);
		return FTH_FALSE;
	}

	// if word exists, put its xt on the stack
	DictionaryEntry *pDict = (DictionaryEntry*)pFirth->head;

	for (; pDict; pDict = pDict->next)
	{
		if (!pDict->flags.hidden && !_stricmp(pDict->name, word))
		{
			// word found so pop the input addr and push xt for the word onto the stack
			fth_pop(pFirth);
			fth_push(pFirth, (FirthNumber)pDict);
			return fth_push(pFirth, FTH_TRUE);
		}
	}

	fth_push(pFirth, FTH_FALSE);
	return FTH_FALSE;
}

// implements TICK word
// Note: if word not found in dictionary, leaves the input addr on the stack
static int fth_tick(FirthState *pFirth)
{
	// get a WORD from input
	fth_push(pFirth, ' ');
	fth_word(pFirth);

	char *word = (char*)fth_peek(pFirth);

	// if buffer is empty return
	if (0 == *word)
		return FTH_FALSE;

	// if word exists, put its xt on the stack
	DictionaryEntry *pDict = (DictionaryEntry*)pFirth->head;

	for (; pDict; pDict = pDict->next)
	{
		if (!pDict->flags.hidden && !_stricmp(pDict->name, word))
		{
			// word found so pop the input addr and push xt for the word onto the stack
			fth_pop(pFirth);
			return fth_push(pFirth, (FirthNumber)pDict);
		}
	}

	return FTH_FALSE;
}

// execute a colon word
static int fth_address_interpreter(FirthState *pFirth)
{
	// push IP on return stack
	fth_r_push(pFirth, (int)pFirth->IP);

	// get the xt from the stack
	DictionaryEntry *pThisWord = (DictionaryEntry*)fth_pop(pFirth);

	// use the xt to set the IP
	pFirth->IP = fth_body_internal(pThisWord);

	while (*pFirth->IP)
	{
		DictionaryEntry *pNextWord = (DictionaryEntry*)*pFirth->IP++;

		// if we are going to call a colon word, put the xt on the stack
		if (pNextWord->flags.colon_word || pNextWord->flags.xt_on_stack)
			fth_push(pFirth, (FirthNumber)pNextWord);

		FirthFunc f = (FirthFunc)pNextWord->code_pointer;
		f(pFirth);
	}

	// pop IP off return stack
	pFirth->IP = (int*)fth_r_pop(pFirth);

	return FTH_TRUE;
}

// implements ':' - begin compiling a new word
static int fth_colon(FirthState *pFirth)
{
	// create a new dictionary entry
	fth_create(pFirth);

	pFirth->head->code_pointer = fth_address_interpreter;
	pFirth->head->flags.colon_word = 1;
	pFirth->head->flags.hidden = 1;				// new WORDS are not visible until fully defined

	pFirth->compiling = true;
	return FTH_TRUE;
}

// implements ';' - end definition of a new WORD
static int fth_semicolon(FirthState *pFirth)
{
	// mark the EXIT of this word
	fth_write_to_cp(pFirth, 0);

	pFirth->head->flags.hidden = 0;				// new WORDS become visible only when fully defined

	pFirth->compiling = false;
	return FTH_TRUE;
}

// implementes COMPILE - compile the words in a new colon definition
static int fth_compile(FirthState *pFirth)
{
	LOG("COMPILE");

	// inner interpreter loop
	while (pFirth->compiling)
	{
		// if result is TRUE then there is an xt on the stack
		// otherwise the addr of an input that is not a WORD
		FirthNumber result = fth_pop(pFirth);

		// get the next word
		if (result == FTH_TRUE)
		{
			// get the xt of the word to compile
			DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pFirth);

			// if it is an immediate word execute it, otherwise thread it
			if (pDict->flags.immediate)
			{
				FirthFunc f = (FirthFunc)pDict->code_pointer;
				f(pFirth);
			}
			else
			{
				// write instruction to new colon word definition
				fth_write_to_cp(pFirth, (FirthNumber)pDict);
			}
		}
		else
		{
			// the addr of the "word" is now on the stack
			char *input = (char*)fth_peek(pFirth);

			if (input[0] == '\0')
			{
				// empty string on the stack, discard it!!
				fth_pop(pFirth);

				// (re-)fill input bufer
				fth_fill_tib(pFirth);
			} 
			else if (isInteger(input))
			{
				DictionaryEntry *xt = fth_tick_internal(pFirth, "LITERAL");
				fth_write_to_cp(pFirth, (FirthNumber)xt);
				
				// pop the addr off the stack, convert it to a number and put back on stack
				fth_number(pFirth);
				fth_write_to_cp(pFirth, fth_pop(pFirth));
			}
			else if (isdigit(input[0]) && (strchr(input, '.') || strchr(input, 'e')))
			{
				DictionaryEntry *xt = fth_tick_internal(pFirth, "FLITERAL");
				fth_write_to_cp(pFirth, (FirthNumber)xt);
				
				// pop the addr off the stack, convert it to a number and put back on stack
				fth_number(pFirth);
				FirthFloat f = fth_popf(pFirth);
				fth_write_to_cp(pFirth, *(int*)&f);
			}
			else
			{
				// unknown WORD
				// TODO - stop compiling?
				char *s = (char*)fth_pop(pFirth);
				firth_printf(pFirth, "%s ?\n", s);
				break;
			}
		}

		// look for the next word
		if (pFirth->compiling)
			fth_interpreter_tick(pFirth);
	}

	return FTH_TRUE;
}

// implements EXECUTE - execute the xt on TOS
// ( xt -- )
static int fth_execute(FirthState *pFirth)
{
	LOG("EXECUTE");

	DictionaryEntry *pDict = (DictionaryEntry*)fth_peek(pFirth);

	// do not execute compile only words
	if ( pDict->flags.compile_only )
	{
		pFirth->firth_print(" action is not a function\n");
		return FTH_FALSE;
	}

	// leave the xt on the stack only if it is a colon word or a defining word
	if ( !pDict->flags.colon_word && !pDict->flags.xt_on_stack )
	{
		fth_pop(pFirth);
	}

	// execute the word
	FirthFunc f = pDict->code_pointer;
	return f(pFirth);
}

// implements CREATE
// create a new empty dictionary entry
static int fth_create(FirthState *pFirth)
{
	// get the next word in input stream
	fth_push(pFirth, ' ');
	fth_word(pFirth);

	char *newWord = (char*)fth_pop(pFirth);

	return fth_make_dict_entry(pFirth, newWord);
}

// implements INTERPRET
static int fth_interpret(FirthState *pFirth)
{
	LOG("INTERPRET");

	while (true)
	{
		// read input and try to find a word
		fth_interpreter_tick(pFirth);

		if (pFirth->compiling)
		{
			fth_compile(pFirth);
		}
		else
		{
			FirthNumber result = fth_pop(pFirth);

			if (result == FTH_TRUE)
			{
				// TICK found an existing word, 
				// the xt is on the stack, EXECUTE it
				fth_execute(pFirth);
			}
			else if (!fth_number(pFirth))
			{
				char *s = (char*)fth_pop(pFirth);

				// if there was text but we don't understand it
				if (0 != *s)
					firth_printf(pFirth, "%s ?\n", s);
				break;
			}
		}
	}

	return FTH_TRUE;
}

// implements QUIT
static int fth_quit(FirthState *pFirth)
{
	LOG("QUIT");

	// show user prompt
	if (pFirth->BLK == stdin)
		pFirth->firth_print("firth>");

	// TODO - reset return stacks?
	assert(pFirth->RP == pFirth->return_stack);

	// fill input bufer
	fth_fill_tib(pFirth);

	fth_interpret(pFirth);

	if (pFirth->BLK == stdin)
		pFirth->firth_print(" ok\n");

	return FTH_TRUE;
}

// implements IMMEDIATE
// make the last word created immediate
static int fth_immediate(FirthState *pFirth)
{
	pFirth->head->flags.immediate = 1;
	return FTH_TRUE;
}

// implements HIDE
// make the last word created hidden
static int fth_hide(FirthState *pFirth)
{
	pFirth->head->flags.hidden = 1;
	return FTH_TRUE;
}

// implements REVEAL
static int fth_reveal(FirthState *pFirth)
{
	pFirth->head->flags.hidden = 0;
	return FTH_TRUE;
}

// implements CELLS
// computes the dictionary space needed for N cells
static int fth_cells(FirthState *pFirth)
{
	FirthNumber n = fth_pop(pFirth);
	return fth_push(pFirth, n * sizeof(FirthNumber));
}

// implements ALLOT
// reserve N bytes in the dictionary
static int fth_allot(FirthState *pFirth)
{
	int n = fth_pop(pFirth);
	pFirth->CP += n;

	return FTH_TRUE;
}

// implements ']'
// ( -- )
static int fth_stop_compile(FirthState *pFirth)
{
	pFirth->compiling = 0;
	return FTH_TRUE;
}

// implements '['
// ( -- )
static int fth_start_compile(FirthState *pFirth)
{
	pFirth->compiling = 1;
	return FTH_TRUE;
}

// implements .S
static int fth_dots(FirthState *pFirth)
{
	// make a copy of the stack
	FirthNumber *s = pFirth->SP;

	pFirth->firth_print("Stack Top -> [ ");
	while (s-- != pFirth->stack)
	{
		if (pFirth->hexmode)
			firth_printf(pFirth, "0x%0X ", *s);
		else
			firth_printf(pFirth, "%d ", *s);
	}
	pFirth->firth_print("]\n");

	return FTH_TRUE;
}

// implements DEPTH - push the current stack size onto the stack
// ( -- n )
static int fth_depth(FirthState *pFirth)
{
	FirthNumber depth = (pFirth->SP - pFirth->stack);
	fth_push(pFirth, depth);
	return FTH_TRUE;
}

// run-time behavior of a marker - reset dictionary state to the marker
static int fth_marker_imp(FirthState *pFirth)
{
	DictionaryEntry *xt = (DictionaryEntry*)fth_pop(pFirth);

	// reset dictionary head
	pFirth->head = xt->next;

	// reset CP
	pFirth->CP = (BYTE*)xt;
	return FTH_TRUE;
}

// implements MARKER - create a state marker in the dictionary
// ( -- )
static int fth_marker(FirthState *pFirth)
{
	// create a new dictionary entry for this marker
	fth_create(pFirth);

	pFirth->head->code_pointer = fth_marker_imp;
	pFirth->head->flags.xt_on_stack = 1;

	return FTH_TRUE;
}

// implements ','
// ( n -- )
static int fth_comma(FirthState *pFirth)
{
	FirthNumber n = fth_pop(pFirth);
	return fth_write_to_cp(pFirth, n);
}

// implements BRANCH
// ( -- )
static int fth_branch(FirthState *pFirth)
{
	// get addr
	FirthNumber *addr = pFirth->IP;
	pFirth->IP = (FirthNumber*)*addr;
	return FTH_TRUE;
}

// implements BRANCH?
// ( f -- )
static int fth_conditional_branch(FirthState *pFirth)
{
	FirthNumber n = fth_pop(pFirth);

	// branch if n == 0
	if (0 == n)
	{
		FirthNumber addr = *pFirth->IP;
		pFirth->IP = (FirthNumber*)addr;
	}
	else
		pFirth->IP++;

	return FTH_TRUE;
}

// implements '('
// ( -- )
static int fth_left_parens(FirthState *pFirth)
{
	fth_push(pFirth, ')');
	fth_word(pFirth);
	return fth_pop(pFirth);
}

// implements '\'
// ( -- )
static int fth_backslash(FirthState *pFirth)
{
	fth_push(pFirth, '\n');
	fth_word(pFirth);
	return fth_pop(pFirth);
}

// helper for LOAD
// ( -- )
static int load_helper(FirthState *pFirth, char *filename)
{
	FILE *f = fopen(filename, "rt");
	if (!f)
	{
		pFirth->firth_print("File not found.\n");
		return FTH_FALSE;
	}

	pFirth->BLK = f;
	
	return FTH_TRUE;
}

// implements INCLUDE
// ( -- )
static int fth_load(FirthState *pFirth)
{
	char include_file[FTH_MAX_PATH];
	
	// get the filename
	fth_push(pFirth, ' ');
	fth_word(pFirth);

	char *filename = (char*)fth_pop(pFirth);

	// see if filename was already loaded
	sprintf(include_file, "%s-included", filename);
	DictionaryEntry *pEntry = fth_tick_internal(pFirth, include_file);

	if (pEntry)
	{
		pFirth->firth_print("File already loaded.\n");
		return FTH_TRUE;
	}

	fth_make_dict_entry(pFirth, include_file);

	pFirth->head->code_pointer = fth_marker_imp;
	pFirth->head->flags.xt_on_stack = 1;

	return load_helper(pFirth, filename);
}

// implements R>
// (R: n -- ) ( -- n)
static int fth_from_r(FirthState *pFirth)
{
	FirthNumber n = fth_r_pop(pFirth);
	return fth_push(pFirth, n);
}

// implements >R
// ( n -- ) (R: -- n)
static int fth_to_r(FirthState *pFirth)
{
	FirthNumber n = fth_pop(pFirth);
	return fth_r_push(pFirth, n);
}

// implements IF
// ( f -- )
static int fth_if(FirthState *pFirth)
{
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "BRANCH?"));
	fth_push(pFirth, (FirthNumber)pFirth->CP);
	fth_write_to_cp(pFirth, -1);

	return FTH_TRUE;
}

// implements THEN
// ( -- )
static int fth_then(FirthState *pFirth)
{
	FirthNumber *addr = (FirthNumber*)fth_pop(pFirth);
	*addr = (FirthNumber) pFirth->CP;

	return FTH_TRUE;
}

// implements ELSE
// ( -- )
static int fth_else(FirthState *pFirth)
{
	FirthNumber *addr = (FirthNumber*)fth_pop(pFirth);
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "BRANCH"));
	fth_push(pFirth, (FirthNumber)pFirth->CP);
	fth_write_to_cp(pFirth, -1);

	*addr = (FirthNumber)pFirth->CP;
	
	return FTH_TRUE;
}

// implements BEGIN
// ( -- )
static int fth_begin(FirthState *pFirth)
{
	// push current code pointer onto the stack
	return fth_push(pFirth, (FirthNumber)pFirth->CP);
}

// implements AGAIN
// ( -- )
static int fth_again(FirthState *pFirth)
{
	FirthNumber addr = fth_pop(pFirth);
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "BRANCH"));
	fth_write_to_cp(pFirth, addr);

	return FTH_TRUE;
}

// implements UNTIL
// ( f -- )
static int fth_until(FirthState *pFirth)
{
	FirthNumber addr = fth_pop(pFirth);
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "BRANCH?"));
	fth_write_to_cp(pFirth, addr);

	return FTH_TRUE;
}

// implements WHILE
// ( f -- )
static int fth_while(FirthState *pFirth)
{
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "BRANCH?"));
	fth_push(pFirth, (FirthNumber)pFirth->CP);
	fth_write_to_cp(pFirth, -1);

	return FTH_TRUE;
}

// implements REPEAT
// ( -- )
static int fth_repeat(FirthState *pFirth)
{
	FirthNumber *while_addr = (FirthNumber*)fth_pop(pFirth);

	// compile-time behavior
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "BRANCH"));
	FirthNumber dest = fth_pop(pFirth);
	fth_write_to_cp(pFirth, dest);
	*while_addr = (FirthNumber)pFirth->CP;

	return FTH_TRUE;
}

// implements DO
// ( limit index -- )
static int fth_do(FirthState *pFirth)
{
	// push current code pointer onto the stack
	fth_push(pFirth, (FirthNumber)pFirth->CP);

	// save limit and index on return stack
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "SWAP"));
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, ">R"));
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, ">R"));
	
	return FTH_TRUE;
}

// implements LOOP
// ( -- )
static int fth_loop(FirthState *pFirth)
{
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "R>")); // get index from return stack
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "R>")); // get limit from return stack
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "SWAP"));

	// inc index by 1
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "1+"));

	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "OVER")); // make copy of limit index
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "OVER")); // one to test, other goes back on return stack

	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "="));		// we consume one copy with test
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "BRANCH?"));	// conditional branch back to DO

	// get TOS addr for branch target
	FirthNumber dest = fth_pop(pFirth);
	fth_write_to_cp(pFirth, dest);

	// drop limit index from stack
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "DROP"));
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "DROP"));

	return FTH_TRUE;
}

// implements +LOOP
// ( n -- )
static int fth_ploop(FirthState *pFirth)
{
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "R>")); // get index from return stack
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "R>")); // get limit from return stack
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "SWAP"));

	// increment limit index -> limit index increment
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "ROT"));
	
	// add increment to index
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "+"));

	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "OVER")); // make copy of limit index
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "OVER")); // one to test, other goes back on return stack

	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "="));		// we consume one copy with test
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "BRANCH?"));	// conditional branch back to DO

	// get TOS addr for branch target
	FirthNumber dest = fth_pop(pFirth);
	fth_write_to_cp(pFirth, dest);

	// drop limit index from stack
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "DROP"));
	fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "DROP"));

	return FTH_TRUE;
}

// run-time behavior of (.")
static int fth_dot_quote_imp(FirthState *pFirth)
{
	firth_printf(pFirth, "%s\n", (char*)pFirth->IP);

	// advance IP past string
	char *p = (char*)pFirth->IP;
	p += strlen((char*)pFirth->IP) + 1;
	pFirth->IP = (FirthNumber*)p;

	return FTH_TRUE;
}

// implements ."
static int fth_dot_quote(FirthState *pFirth)
{
	// get the string
	fth_push(pFirth, '"');
	fth_word(pFirth);

	char *pStr = (char*)fth_pop(pFirth);

	if (pFirth->compiling)
	{
		fth_write_to_cp(pFirth, (FirthNumber)fth_tick_internal(pFirth, "(.\")"));
		while (*pStr)
			*pFirth->CP++ = *pStr++;
		*pFirth->CP++ = 0;
	}
	else
		firth_printf(pFirth, "%s\n", pStr);

	return FTH_TRUE;
}

// implements J ( -- n )
static int fth_j(FirthState *pFirth)
{
	FirthNumber n = *(pFirth->RP - 3);
	return fth_push(pFirth, n);
}

// implements POSTPONE
static int fth_postpone(FirthState *pFirth)
{
	// get the next word
	fth_push(pFirth, ' ');
	fth_word(pFirth);

	// find the word in the dictionary
	char *oldWordName = (char*)fth_pop(pFirth);
	DictionaryEntry *pDict = fth_tick_internal(pFirth, oldWordName);
	if (!pDict)
	{
		// old word not found!
		pFirth->firth_print(oldWordName);
		pFirth->firth_print(" word not found!\n");
		return FTH_FALSE;
	}

	// append the compilation semantics
	// write instruction to new colon word definition
	fth_write_to_cp(pFirth, (FirthNumber)pDict);

	return FTH_TRUE;
}

// implements [']
// : ['] ( compilation: "name" --; run-time: -- xt ) ' POSTPONE literal ; immediate
static int fth_bracket_tick(FirthState *pFirth)
{

	return FTH_TRUE;
}

// implements
//static int fth_xxx(FirthState *pFirth)
//{
//
//	return FTH_TRUE;
//}

//
// Note: Since word lookup is currently O(n) on dictionary size
// try to keep common words towards the bottom for speed
//
static const FirthWordSet basic_lib[] =
{
	{ "BEGIN", fth_begin },
	{ "AGAIN", fth_again },
	{ "UNTIL", fth_until },
	{ "WHILE", fth_while },
	{ "REPEAT", fth_repeat },
	{ "DO", fth_do },
	{ "LOOP", fth_loop },
	{ "+LOOP", fth_ploop },
	{ ".\"", fth_dot_quote },
	{ "(.\")", fth_dot_quote_imp },

//	{ "POSTPONE", fth_postpone },

	{ "BRANCH?", fth_conditional_branch },
	{ "BRANCH", fth_branch },
	{ "IF", fth_if },
	{ "THEN", fth_then },
	{ "ELSE", fth_else },

#if FTH_FIRTH_SYNTAX == 1
	{ "ENDIF", fth_then },
	{ "FOR", fth_do },
	{ "ALLOC", fth_allot },
	{ "CONST", fth_const },
	{ "VAR", fth_var },

	#if FTH_I_LIKE_SWIFT == 1
		{ "FUNC", fth_colon },
	#endif

	#if FTH_I_LIKE_RUST == 1
		{ "FN", fth_colon },
	#endif

	#if FTH_I_LIKE_PYTHON == 1
		{ "DEF", fth_colon },
	#endif

	#if FTH_I_LIKE_JAVASCRIPT == 1
		{ "FUNCTION", fth_colon },
	#endif

#endif

	{ ">BODY", fth_body },
	{ "I", fth_r_fetch },
	{ "J", fth_j },
	{ "R@", fth_r_fetch },
	{ "R>", fth_from_r },
	{ ">R", fth_to_r },
	{ "INCLUDE", fth_load },
	{ "EXECUTE", fth_execute },
	{ "MARKER", fth_marker },
	{ "QUIT", fth_quit},
	{ "NUMBER", fth_number },
	{ "'", fth_tick },
	{ ",", fth_comma },
	{ "WORD", fth_word },

	{ "COMPILE", fth_compile },
	{ "INTERPRET", fth_interpret },
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
	{ "LITERAL", fth_literal },
	
#if FTH_INCLUDE_FLOAT == 1
	{ "FCONST", fth_fconst },
	{ "FCONSTANT", fth_fconst },
	{ "FVAR", fth_fvar },
	{ "FVARIABLE", fth_fvar },
	{ "FLITERAL", fth_fliteral },
#endif

	{ "DEPTH", fth_depth },
	{ ".S", fth_dots },

	{ "KEY", fth_key },
	{ "CELLS", fth_cells },
	{ "ALLOT", fth_allot },

	{ "(", fth_left_parens },
	{ "\\", fth_backslash },

	{ NULL, NULL }
};

// list of words that are IMMEDIATE
static const char *immediate_words[] =
{
	";",
	"[",
	"(",
	"IF",
	"THEN",
#if FTH_FIRTH_SYNTAX == 1
	"ENDIF",
	"FOR",
#endif
	"ELSE",
	"BEGIN",
	"AGAIN",
	"UNTIL",
	"WHILE",
	"REPEAT",
	"DO",
	"LOOP",
	"+LOOP",
	"(",
	"\\",
	".\"",
//	"POSTPONE",
	NULL
};

// list of words that are compile-only
static const char *compile_only_words[] =
{
	";",
	"LITERAL",
	"BRANCH",
	"BRANCH?",
	"IF",
	"THEN",
#if FTH_FIRTH_SYNTAX == 1
	"ENDIF",
	"FOR",
#endif
	"ELSE",
	"BEGIN",
	"AGAIN",
	"UNTIL",
	"WHILE",
	"REPEAT",
	"DO",
	"LOOP",
	"+LOOP",
	"I",
	"J",
//	"POSTPONE",
	NULL
};

//========================
// Public Firth functions
//========================

// allows embedders to set their own output function
int fth_set_output_function(FirthState *pFirth, FirthOutputFunc f)
{
	pFirth->firth_print = f;
	return FTH_TRUE;
}

// C API to create a new forth state
FirthState *fth_create_state()
{
	FirthState *pFirth = malloc(sizeof(FirthState));
	if (!pFirth)
		return NULL;

	pFirth->dictionary_base = malloc(FTH_ENV_SIZE);
	if (!pFirth->dictionary_base)
	{
		free(pFirth);
		return NULL;
	}

	// setup stacks
	pFirth->SP = pFirth->stack;
	pFirth->RP = pFirth->return_stack;

#if FTH_INCLUDE_FLOAT == 1
	pFirth->FP = pFirth->float_stack; 
#endif

	// setup our internal state
	pFirth->CP = pFirth->dictionary_base;			// setup dictionary pointer
	pFirth->IP = NULL;
	pFirth->head = NULL;							// dictionary is empty at this point
	pFirth->firth_print = forth_default_output;		// setup default print routine
	pFirth->halted = 0;
	pFirth->IN = pFirth->TIB;						// TODO - not yet used
	pFirth->compiling = false;						// start in interpreter mode

	pFirth->hexmode = 0;
	pFirth->maxr = 0;
	pFirth->maxs = 0;

	pFirth->BLK = stdin;

	// register built-in words
	fth_register_wordset(pFirth, basic_lib);
	fth_register_core_wordset(pFirth);

	// setup immediate words
	for (const char **word = immediate_words; *word; word++)
		fth_make_immediate(pFirth, *word);

	// setup compile-only words
	for (const char **word = compile_only_words; *word; word++)
		fth_make_compile_only(pFirth, *word);

	// setup defining words
	/* none yet! */

	// setup hidden words
	fth_make_hidden(pFirth, "(.\")");

	// setup variables
	fth_define_word_var(pFirth, "C0", (FirthNumber*)&pFirth->dictionary_base);
	fth_define_word_var(pFirth, "CP", (FirthNumber*)&pFirth->CP);
	fth_define_word_var(pFirth, "RP", (FirthNumber*)&pFirth->RP);
	fth_define_word_var(pFirth, "SP", (FirthNumber*)&pFirth->SP);
	fth_define_word_var(pFirth, ">IN", (FirthNumber*)&pFirth->IN);
	fth_define_word_var(pFirth, "TIB", (FirthNumber*)&pFirth->TIB);
	fth_define_word_var(pFirth, "#TIB", (FirthNumber*)&pFirth->tib_len);

	fth_define_word_var(pFirth, "ENV.MAXS", (FirthNumber*)&pFirth->maxs);
	fth_define_word_var(pFirth, "ENV.MAXR", (FirthNumber*)&pFirth->maxr);
	fth_define_word_var(pFirth, "ENV.HEX", (FirthNumber*)&pFirth->hexmode);

	// load the core library
	load_helper(pFirth, "core.fth");

#if FTH_INCLUDE_FLOAT == 1
	// load (optional) floating point libraries
	firth_register_float(pFirth);
#endif

	return pFirth;
}

// C API to cleanup and free forth state
int fth_delete_state(FirthState *pFirth)
{
	free(pFirth->dictionary_base);
	free(pFirth);

	return FTH_TRUE;
}

// C API to mark the given word as hidden
int fth_make_hidden(FirthState* pFirth, const char *word)
{
	DictionaryEntry *pEntry = fth_tick_internal(pFirth, word);
	if (!pEntry)
	{
		pFirth->firth_print("Word not found.");
		return FTH_FALSE;
	}

	pEntry->flags.hidden = 1;
	return FTH_TRUE;
}

// C API to mark the given word as immediate
int fth_make_immediate(FirthState* pFirth, const char *word)
{
	DictionaryEntry *pEntry = fth_tick_internal(pFirth, word);
	if (!pEntry)
	{
		pFirth->firth_print("Word not found.");
		return FTH_FALSE;
	}

	pEntry->flags.immediate = 1;
	return FTH_TRUE;
}

// C API to mark the given word as compile-only
int fth_make_compile_only(FirthState* pFirth, const char *word)
{
	DictionaryEntry *pEntry = fth_tick_internal(pFirth, word);
	if (!pEntry)
	{
		pFirth->firth_print("Word not found.");
		return FTH_FALSE;
	}

	pEntry->flags.compile_only = 1;
	return FTH_TRUE;
}

// C API to mark the given word as requiring the xt on the stack
int fth_make_xt_required(FirthState* pFirth, const char *word)
{
	DictionaryEntry *pEntry = fth_tick_internal(pFirth, word);
	if (!pEntry)
	{
		pFirth->firth_print("Word not found.");
		return FTH_FALSE;
	}

	pEntry->flags.xt_on_stack = 1;
	return FTH_TRUE;
}

// C API to register a wordset
int fth_register_wordset(FirthState *pFirth, const FirthWordSet words[])
{
	for (int i = 0; words[i].wordName && words[i].func; i++)
	{
		fth_make_dict_entry(pFirth, words[i].wordName);
		pFirth->head->code_pointer = words[i].func;
	}

	return FTH_TRUE;
}

// C API to push a value on the stack
int fth_push(FirthState *pFirth, FirthNumber val)
{
	// check for stack overflow
	if ((pFirth->SP - pFirth->stack) + 1 > FTH_STACK_SIZE)
	{
		pFirth->firth_print("Stack overflow!\n");
		return FTH_FALSE;
	}

	*pFirth->SP++ = val;

	FirthNumber depth = pFirth->SP - pFirth->stack;
	if (depth > pFirth->maxs)
		pFirth->maxs = depth;

	return FTH_TRUE;
}

// C API to pop a value off the stack
FirthNumber fth_pop(FirthState *pFirth)
{
	// check for stack underflow
	if (pFirth->SP <= pFirth->stack)
	{
		pFirth->firth_print("Stack underflow!\n");
		return FTH_FALSE;
	}

	pFirth->SP--;
	return *pFirth->SP;
}

// C API to return the TOS without removing it
int fth_peek(FirthState *pFirth)
{
	if (pFirth->SP == pFirth->stack)
	{
		pFirth->firth_print("Stack is empty!\n");
		return FTH_FALSE;
	}

	return *(pFirth->SP-1);
}

// C API for defining user constants
int fth_define_word_const(FirthState *pFirth, const char *name, FirthNumber val)
{
	fth_make_dict_entry(pFirth, name);

	pFirth->head->code_pointer = fth_const_imp;
	pFirth->head->flags.xt_on_stack = 1;
	pFirth->head->flags.constant = 1;

	// store constant value in dictionary
	fth_write_to_cp(pFirth, val);

	return FTH_TRUE;
}

// C API for defining user variables
int fth_define_word_var(FirthState *pFirth, const char *name, FirthNumber *pVar)
{
	fth_make_dict_entry(pFirth, name);

	pFirth->head->code_pointer = fth_user_var_imp;
	pFirth->head->flags.xt_on_stack = 1;
	pFirth->head->flags.variable = 1;

	// store var pointer in dictionary
	fth_write_to_cp(pFirth, (FirthNumber)pVar);

	return FTH_TRUE;
}

#if FTH_INCLUDE_FLOAT == 1
// C API for defining float user constants
int fth_define_word_fconst(FirthState *pFirth, const char *name, FirthFloat val)
{
	fth_make_dict_entry(pFirth, name);

	pFirth->head->code_pointer = fth_fconst_imp;
	pFirth->head->flags.xt_on_stack = 1;
	pFirth->head->flags.constant = 1;

	// store constant value in dictionary
	fth_write_to_cp(pFirth, (FirthNumber)*(int*)&val);

	return FTH_TRUE;
}

// C API for defining user variables
int fth_define_word_fvar(FirthState *pFirth, const char *name, FirthFloat *pVar)
{
	fth_make_dict_entry(pFirth, name);

	pFirth->head->code_pointer = fth_user_fvar_imp;
	pFirth->head->flags.xt_on_stack = 1;
	pFirth->head->flags.variable = 1;

	// store var pointer in dictionary
	fth_write_to_cp(pFirth, (FirthNumber)pVar);

	return FTH_TRUE;
}
#endif

// C API for the outer interpreter
int fth_update(FirthState *pFirth)
{
	LOG("UPDATE");

	return fth_quit(pFirth);
}

// C API for parsing/compiling/executing a string of Firth
int fth_parse_string(FirthState *pFirth, const char *str)
{
	if (!str)
		return FTH_FALSE;

	int len = strlen(str);
	if (len > FTH_MAX_WORD_NAME)
		return FTH_FALSE;

	strcpy(pFirth->TIB, str);
	pFirth->tib_len = len;

	fth_interpret(pFirth);

	return FTH_TRUE;
}

// C API for executing an existing word
int fth_exec_word(FirthState *pFirth, const char *word)
{
	DictionaryEntry *pEntry = fth_tick_internal(pFirth, word);
	fth_push(pFirth, (FirthNumber)pEntry);
	return fth_execute(pFirth);
}

// C API convenience function for executing words
int fth_exec_word1(FirthState *pFirth, const char *word, FirthNumber n) 
{ 
	fth_push(pFirth, n); 
	return fth_exec_word(pFirth, word);
}

// C API convenience function for executing words
int fth_exec_word2(FirthState *pFirth, const char *word, FirthNumber n1, FirthNumber n2)
{
	fth_push(pFirth, n1);
	fth_push(pFirth, n2);
	return fth_exec_word(pFirth, word);
}

// C API convenience function for executing words
int fth_exec_word3(FirthState *pFirth, const char *word, FirthNumber n1, FirthNumber n2, FirthNumber n3)
{
	fth_push(pFirth, n1);
	fth_push(pFirth, n2);
	fth_push(pFirth, n3);
	return fth_exec_word(pFirth, word);
}

// C API to retrieve address of Firth Word variable
FirthNumber *fth_get_var(FirthState *pFirth, const char *word)
{
	DictionaryEntry *pEntry = fth_tick_internal(pFirth, word);
	if (!pEntry)
		return NULL;

	return fth_body_internal(pEntry);
}
