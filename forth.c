#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#include "forth.h"

//
bool true = ~0;
bool false = 0;

static DictionaryEntry *fth_tick_internal(ForthState *pForth, const char *word);

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

// given an xt get the data pointer
static int* fth_body_internal(DictionaryEntry *xt)
{
	return (int*)(xt->name + xt->name_len + 1);
}

// implements >BODY ( addr -- d )
static int fth_body(ForthState *pForth)
{
	DictionaryEntry *xt = (DictionaryEntry *)fth_pop(pForth);
	return fth_push(pForth, (ForthNumber)(int*)(xt->name + xt->name_len + 1));
}

// push a value onto the return stack
static int fth_r_push(ForthState *pForth, ForthNumber val)
{
	// check for return stack overflow
	if ((pForth->RP - pForth->return_stack) + 1 > FTH_RETURN_STACK_SIZE)
	{
		pForth->forth_print("Return Stack overflow!\n");
		return FTH_FALSE;
	}

	*pForth->RP++ = val;

	ForthNumber depth = pForth->RP - pForth->return_stack;
	if (depth > pForth->maxr)
		pForth->maxr = depth;

	return FTH_TRUE;
}

// pop a value off the return stack
static ForthNumber fth_r_pop(ForthState *pForth)
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
static ForthNumber fth_r_fetch(ForthState *pForth)
{
	// check for stack underflow
	if (pForth->RP <= pForth->return_stack)
	{
		pForth->forth_print("Return Stack underflow!\n");
		return FTH_FALSE;
	}

	return fth_push(pForth, *(pForth->RP - 1));
}

// implements KEY - get a new key from the terminal
static int fth_key(ForthState *pForth)
{
	return fth_push(pForth, getc(stdin));
}

// implements ACCEPT - read in a line of text
// ( addr limit -- size )
static int fth_accept(ForthState *pForth)
{
	int c;
	ForthNumber count = 0;

	ForthNumber limit = fth_pop(pForth);
	ForthNumber addr = fth_pop(pForth);
	
	char *p = (char*)addr;

	// read at most limit characters from the input and store at addr
	while ((c = getc(pForth->BLK)) != '\n' && count < limit)
	{
		*p++ = c;
		count++;
	}

	// make sure input string is null terminated
	memset(p, 0, FTH_MAX_WORD - (p - pForth->TIB));

	pForth->IN = pForth->TIB;

	fth_push(pForth, count);

	return FTH_TRUE;
}

// implements WORD - 
// ( delim -- addr )
static int fth_word(ForthState *pForth)
{
	int c;

	int delim = fth_pop(pForth);

	// skip any leading delimiter chars
	while ((c = *pForth->IN) == delim)
		pForth->IN++;

	// start with the current input pointer
	char *p = pForth->IN;

	// at this point c contains a non delimiter character

	// read until delimiter, EOL, EOF or buffer end
	while ( (p - pForth->TIB < pForth->tib_len) && c != 0 && c != delim && c != '\n' && c != EOF )
	{
		p++;
		c = *p;
	}

	if (c == EOF)
	{
		if (pForth->BLK != stdin)
		{
			// close previous file
			fclose(pForth->BLK);

			// set input to terminal
			pForth->BLK = stdin;
		}
	}

	// make word in TIB asciiz
	*p = '\0';

	// put addr of word on stack
	fth_push(pForth, (ForthNumber)pForth->IN);

	// advance IN pointer to beyond the word
	pForth->IN = p + 1;

	return FTH_TRUE;
}

// create a new empty dictionary entry withe name given by word
static int fth_make_dict_entry(ForthState *pForth, const char *word)
{
	// see if the word already exists and if so warn about it
	DictionaryEntry *pEntry = (DictionaryEntry*)fth_tick_internal(pForth, word);
	if (pEntry)
	{
		forth_printf(pForth, "Note: redefining an existing word (%s).\n", pEntry->name);
	}

	DictionaryEntry *pNewHead = (DictionaryEntry *)pForth->CP;
	pNewHead->code_pointer = NULL;
	pNewHead->next = pForth->head;

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

	pForth->head = pNewHead;
	pForth->CP += sizeof(DictionaryEntry) + pForth->head->name_len + 1;

	return FTH_TRUE;
}

//
static int fth_write_to_cp(ForthState *pForth, ForthNumber val)
{
	*((ForthNumber*)pForth->CP) = val;
	pForth->CP += sizeof(ForthNumber);		// advance dictionary pointer

	return FTH_TRUE;
}

// run-time implementation of CONSTANT
static int fth_const_imp(ForthState *pForth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pForth);

	int val = *fth_body_internal(pDict);
	fth_push(pForth, val);

	return FTH_TRUE;
}

// run-time implementation of VARIABLE
static int fth_var_imp(ForthState *pForth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pForth);

	int addr = (int)fth_body_internal(pDict);
	fth_push(pForth, addr);

	return FTH_TRUE;
}

//
static int fth_user_var_imp(ForthState *pForth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pForth);

	ForthNumber **ppVar = (ForthNumber**)fth_body_internal(pDict);
	fth_push(pForth, (ForthNumber)*ppVar);

	return FTH_TRUE;
}

// create a CONSTANT word
static int fth_const(ForthState *pForth)
{
	// create a new dictionary entry for this constant
	fth_create(pForth);

	// the stack at this point contains | const_val |
	ForthNumber n = fth_pop(pForth);

	pForth->head->code_pointer = fth_const_imp;
	pForth->head->flags.xt_on_stack = 1;
	pForth->head->flags.constant = 1;

	// store constant value in dictionary
	fth_write_to_cp(pForth, n);

	return FTH_TRUE;
}

// create a VARIABLE word
static int fth_var(ForthState *pForth)
{
	// create a new dictionary entry for this variable
	fth_create(pForth);

	pForth->head->code_pointer = fth_var_imp;
	pForth->head->flags.xt_on_stack = 1;
	pForth->head->flags.variable = 1;

	// store value in dictionary
	fth_write_to_cp(pForth, FTH_UNINITIALIZED);

	return FTH_TRUE;
}

// implements NUMBER - look at input and if a number push it onto the stack
// Note: if not a number leaves input addr on stack
static int fth_number(ForthState *pForth)
{
	char *input = (char*)fth_peek(pForth);
	if (!isInteger(input))
		return FTH_FALSE;

	ForthNumber n = atoi(input);
	fth_pop(pForth);

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
static DictionaryEntry *fth_tick_internal(ForthState *pForth, const char *word)
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
// Note: if word not found in dictionary leave the input addr on the stack
static int fth_tick(ForthState *pForth)
{
	// get a WORD from input
	fth_push(pForth, ' ');
	fth_word(pForth);

	char *word = (char*)fth_peek(pForth);

	// if buffer is empty return
	if (0 == *word)
		return FTH_FALSE;

	// if word exists, put its xt on the stack
	DictionaryEntry *pDict = (DictionaryEntry*)pForth->head;

	for (; pDict; pDict = pDict->next)
	{
		if (!pDict->flags.hidden && !_stricmp(pDict->name, word))
		{
			// word found so pop the input addr and push xt for the word onto the stack
			fth_pop(pForth);
			return fth_push(pForth, (ForthNumber)pDict);
		}
	}

	return FTH_FALSE;
}

// execute a colon word
static int fth_address_interpreter(ForthState *pForth)
{
	// push IP on return stack
	fth_r_push(pForth, (int)pForth->IP);

	// get the xt from the stack
	DictionaryEntry *pThisWord = (DictionaryEntry*)fth_pop(pForth);

	// use the xt to set the IP
	pForth->IP = fth_body_internal(pThisWord);

	while (*pForth->IP)
	{
		DictionaryEntry *pNextWord = (DictionaryEntry*)*pForth->IP++;

		// if we are going to call a colon word, put the xt on the stack
		if (pNextWord->flags.colon_word || pNextWord->flags.xt_on_stack)
			fth_push(pForth, (ForthNumber)pNextWord);

		ForthFunc f = (ForthFunc)pNextWord->code_pointer;
		f(pForth);
	}

	// pop IP off return stack
	pForth->IP = (int*)fth_r_pop(pForth);

	return FTH_TRUE;
}

// implements ':' - begin compiling a new word
static int fth_colon(ForthState *pForth)
{
	// create a new dictionary entry
	fth_create(pForth);

	pForth->head->code_pointer = fth_address_interpreter;
	pForth->head->flags.colon_word = 1;
	pForth->head->flags.hidden = 1;				// new WORDS are not visible until fully defined

	pForth->compiling = true;
	return FTH_TRUE;
}

// implements ';' - end definition of a new WORD
static int fth_semicolon(ForthState *pForth)
{
	// mark the EXIT of this word
	fth_write_to_cp(pForth, 0);

	pForth->head->flags.hidden = 0;				// new WORDS become visible only when fully defined

	pForth->compiling = false;
	return FTH_TRUE;
}

// implementes COMPILE - compile the words in a new colon definition
static int fth_compile(ForthState *pForth)
{
	// inner interpreter loop
	while (pForth->compiling)
	{
		// get the next word
		if (fth_tick(pForth))
		{
			// get the xt of the word to compile
			DictionaryEntry *pDict = (DictionaryEntry*)fth_pop(pForth);

			// if it is an immediate word execute it, otherwise thread it
			if (pDict->flags.immediate)
			{
				ForthFunc f = (ForthFunc)pDict->code_pointer;
				f(pForth);
			}
			else
			{
				// write instruction to new colon word definition
				fth_write_to_cp(pForth, (ForthNumber)pDict);
			}
		}
		else
		{
			char *input = (char*)fth_peek(pForth);
			if (isInteger(input))
			{
				DictionaryEntry *xt = fth_tick_internal(pForth, "LITERAL");
				fth_write_to_cp(pForth, (ForthNumber)xt);
				fth_number(pForth);
				fth_write_to_cp(pForth, fth_pop(pForth));
			}
			else
				break;
		}
	}

	return FTH_TRUE;
}

// implements EXECUTE - execute the xt on TOS
static int fth_execute(ForthState *pForth)
{
	DictionaryEntry *pDict = (DictionaryEntry*)fth_peek(pForth);

	// do not execute compile only words
	if ( pDict->flags.compile_only )
	{
		pForth->forth_print(" action is not a function\n");
		return FTH_FALSE;
	}

	// leave the xt on the stack only if it is a colon word or a defining word
	if ( !pDict->flags.colon_word && !pDict->flags.xt_on_stack )
	{
		fth_pop(pForth);
	}

	// execute the word
	ForthFunc f = (ForthFunc)pDict->code_pointer;
	return f(pForth);
}

// implements CREATE
// create a new empty dictionary entry
static int fth_create(ForthState *pForth)
{
	// get the next word in input stream
	fth_push(pForth, ' ');
	fth_word(pForth);

	char *newWord = (char*)fth_pop(pForth);

	return fth_make_dict_entry(pForth, newWord);
}

// implements QUIT
static int fth_quit(ForthState *pForth)
{
	// show user prompt
	if (pForth->BLK == stdin)
		pForth->forth_print("firth>");

	// TODO - reset return stacks?
	assert(pForth->RP == pForth->return_stack);

	// fill input bufer
	fth_push(pForth, (ForthNumber)pForth->TIB);
	fth_push(pForth, sizeof(pForth->TIB) - 1);
	fth_accept(pForth);
	pForth->tib_len = fth_pop(pForth);

	while (true)
	{
		// read input and try to find a word
		if (fth_tick(pForth))
		{
			// the WORD exists, execute it
			ForthNumber f = fth_execute(pForth);
			if (!f)
				return FTH_TRUE;

			// if we are now compiling continue
			if (pForth->compiling)
				fth_compile(pForth);
		}
		else if (!fth_number(pForth))
		{
			char *s = (char*)fth_pop(pForth);
			if (0 != *s)
				forth_printf(pForth, "%s ?\n", s);
			break;
		}
	}

	if (pForth->BLK == stdin)
		pForth->forth_print(" ok\n");

	return FTH_TRUE;
}

// implements IMMEDIATE
// make the last word created immediate
static int fth_immediate(ForthState *pForth)
{
	pForth->head->flags.immediate = 1;
	return FTH_TRUE;
}

// implements HIDE
// make the last word created hidden
static int fth_hide(ForthState *pForth)
{
	pForth->head->flags.hidden = 1;
	return FTH_TRUE;
}

// implements REVEAL
static int fth_reveal(ForthState *pForth)
{
	pForth->head->flags.hidden = 0;
	return FTH_TRUE;
}

// implements CELLS
// computes the dictionary space needed for N cells
static int fth_cells(ForthState *pForth)
{
	ForthNumber n = fth_pop(pForth);
	return fth_push(pForth, n * sizeof(ForthNumber));
}

// implements ALLOT
// reserve N bytes in the dictionary
static int fth_allot(ForthState *pForth)
{
	int n = fth_pop(pForth);
	pForth->CP += n;

	return FTH_TRUE;
}

// implements ']'
// ( -- )
static int fth_stop_compile(ForthState *pForth)
{
	pForth->compiling = 0;
	return FTH_TRUE;
}

// implements '['
// ( -- )
static int fth_start_compile(ForthState *pForth)
{
	pForth->compiling = 1;
	return FTH_TRUE;
}

// implements .S
static int fth_dots(ForthState *pForth)
{
	// make a copy of the stack
	ForthNumber *s = pForth->SP;

	pForth->forth_print("Top -> [ ");
	while (s-- != pForth->stack)
	{
		if (pForth->hexmode)
			forth_printf(pForth, "0x%0X ", *s);
		else
			forth_printf(pForth, "%d ", *s);
	}
	pForth->forth_print("]\n");

	return FTH_TRUE;
}

// implements DEPTH - push the current stack size onto the stack
// ( -- n )
static int fth_depth(ForthState *pForth)
{
	ForthNumber depth = (pForth->SP - pForth->stack);
	fth_push(pForth, depth);
	return FTH_TRUE;
}

// run-time behavior of a marker - reset dictionary state to the marker
static int fth_marker_imp(ForthState *pForth)
{
	DictionaryEntry *xt = (DictionaryEntry*)fth_pop(pForth);

	// reset dictionary head
	pForth->head = xt->next;

	// reset CP
	pForth->CP = (BYTE*)xt;
	return FTH_TRUE;
}

// implements MARKER - create a state marker in the dictionary
// ( -- )
static int fth_marker(ForthState *pForth)
{
	// create a new dictionary entry for this marker
	fth_create(pForth);

	pForth->head->code_pointer = fth_marker_imp;
	pForth->head->flags.xt_on_stack = 1;

	return FTH_TRUE;
}

// implements ','
// ( n -- )
static int fth_comma(ForthState *pForth)
{
	ForthNumber n = fth_pop(pForth);
	return fth_write_to_cp(pForth, n);
}

// implements BRANCH
// ( -- )
static int fth_branch(ForthState *pForth)
{
	// get addr
	ForthNumber *addr = pForth->IP;
	pForth->IP = (ForthNumber*)*addr;
	return FTH_TRUE;
}

// implements BRANCH?
// ( f -- )
static int fth_conditional_branch(ForthState *pForth)
{
	ForthNumber n = fth_pop(pForth);

	// branch if n == 0
	if (0 == n)
	{
		ForthNumber addr = *pForth->IP;
		pForth->IP = (ForthNumber*)addr;
	}
	else
		pForth->IP++;

	return FTH_TRUE;
}

// implements '('
// ( -- )
static int fth_left_parens(ForthState *pForth)
{
	fth_push(pForth, ')');
	fth_word(pForth);
	return fth_pop(pForth);
}

// implements '\'
// ( -- )
static int fth_backslash(ForthState *pForth)
{
	fth_push(pForth, '\n');
	fth_word(pForth);
	return fth_pop(pForth);
}

// helper for LOAD
// ( -- )
static int load_helper(ForthState *pForth, char *filename)
{
	FILE *f = fopen(filename, "rt");
	if (!f)
	{
		pForth->forth_print("File not found.\n");
		return FTH_FALSE;
	}

	pForth->BLK = f;
	
	return FTH_TRUE;
}

// implements LOAD
// ( -- )
static int fth_load(ForthState *pForth)
{
	// get the filename
	fth_push(pForth, ' ');
	fth_word(pForth);

	char *filename = (char*)fth_pop(pForth);
	return load_helper(pForth, filename);
}

// implements R>
// (R: n -- ) ( -- n)
static int fth_from_r(ForthState *pForth)
{
	ForthNumber n = fth_r_pop(pForth);
	return fth_push(pForth, n);
}

// implements >R
// ( n -- ) (R: -- n)
static int fth_to_r(ForthState *pForth)
{
	ForthNumber n = fth_pop(pForth);
	return fth_r_push(pForth, n);
}

// implements IF
// ( f -- )
static int fth_if(ForthState *pForth)
{
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "BRANCH?"));
	fth_push(pForth, (ForthNumber)pForth->CP);
	fth_write_to_cp(pForth, -1);

	return FTH_TRUE;
}

// implements THEN
// ( -- )
static int fth_then(ForthState *pForth)
{
	ForthNumber *addr = (ForthNumber*)fth_pop(pForth);
	*addr = (ForthNumber) pForth->CP;

	return FTH_TRUE;
}

// implements ELSE
// ( -- )
static int fth_else(ForthState *pForth)
{
	ForthNumber *addr = (ForthNumber*)fth_pop(pForth);
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "BRANCH"));
	fth_push(pForth, (ForthNumber)pForth->CP);
	fth_write_to_cp(pForth, -1);

	*addr = (ForthNumber)pForth->CP;
	
	return FTH_TRUE;
}

// implements BEGIN
// ( -- )
static int fth_begin(ForthState *pForth)
{
	// push current code pointer onto the stack
	return fth_push(pForth, (ForthNumber)pForth->CP);
}

// implements AGAIN
// ( -- )
static int fth_again(ForthState *pForth)
{
	ForthNumber addr = fth_pop(pForth);
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "BRANCH"));
	fth_write_to_cp(pForth, addr);

	return FTH_TRUE;
}

// implements UNTIL
// ( f -- )
static int fth_until(ForthState *pForth)
{
	ForthNumber addr = fth_pop(pForth);
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "BRANCH?"));
	fth_write_to_cp(pForth, addr);

	return FTH_TRUE;
}

// implements WHILE
// ( f -- )
static int fth_while(ForthState *pForth)
{
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "BRANCH?"));
	fth_push(pForth, (ForthNumber)pForth->CP);
	fth_write_to_cp(pForth, -1);

	return FTH_TRUE;
}

// implements REPEAT
// ( -- )
static int fth_repeat(ForthState *pForth)
{
	ForthNumber *while_addr = (ForthNumber*)fth_pop(pForth);

	// compile-time behavior
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "BRANCH"));
	ForthNumber dest = fth_pop(pForth);
	fth_write_to_cp(pForth, dest);
	*while_addr = (ForthNumber)pForth->CP;

	return FTH_TRUE;
}

// implements DO
// ( limit index -- )
static int fth_do(ForthState *pForth)
{
	// push current code pointer onto the stack
	fth_push(pForth, (ForthNumber)pForth->CP);

	// save limit and index on return stack
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "SWAP"));
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, ">R"));
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, ">R"));
	
	return FTH_TRUE;
}

// implements LOOP
// ( -- )
static int fth_loop(ForthState *pForth)
{
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "R>")); // get index from return stack
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "R>")); // get limit from return stack
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "SWAP"));

	// inc index by 1
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "1+"));

	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "OVER")); // make copy of limit index
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "OVER")); // one to test, other goes back on return stack

	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "="));		// we consume one copy with test
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "BRANCH?"));	// conditional branch back to DO

	// get TOS addr for branch target
	ForthNumber dest = fth_pop(pForth);
	fth_write_to_cp(pForth, dest);

	// drop limit index from stack
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "DROP"));
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "DROP"));

	return FTH_TRUE;
}

// implements +LOOP
// ( n -- )
static int fth_ploop(ForthState *pForth)
{
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "R>")); // get index from return stack
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "R>")); // get limit from return stack
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "SWAP"));

	// increment limit index -> limit index increment
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "ROT"));
	
	// add increment to index
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "+"));

	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "OVER")); // make copy of limit index
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "OVER")); // one to test, other goes back on return stack

	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "="));		// we consume one copy with test
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "BRANCH?"));	// conditional branch back to DO

	// get TOS addr for branch target
	ForthNumber dest = fth_pop(pForth);
	fth_write_to_cp(pForth, dest);

	// drop limit index from stack
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "DROP"));
	fth_write_to_cp(pForth, (ForthNumber)fth_tick_internal(pForth, "DROP"));

	return FTH_TRUE;
}

// run-time behavior of .S
static int fth_dot_quote_imp(ForthState *pForth)
{

}

// implements ."
static int fth_dot_quote(ForthState *pForth)
{
	// get the string
	fth_push(pForth, '"');
	fth_word(pForth);

	char *pStr = (char*)fth_pop(pForth);

	if (pForth->compiling)
	{
		char *p = pForth->CP;
		while (*p)
			*pForth->CP++ = *p++;
		*p = 0;
	}
	else
		pForth->forth_print(pStr);

	//w.data_addr = (int)strdup(lval);
//	emit(OP_SPRINT);
//	emit((FirthInstruction)_strdup(lval));

	return FTH_TRUE;
}

// implements J ( -- n )
static int fth_j(ForthState *pForth)
{
	ForthNumber n = *(pForth->RP - 3);
	return fth_push(pForth, n);
}

// implements POSTPONE
static int fth_postpone(ForthState *pForth)
{
	// get the next word
	fth_push(pForth, ' ');
	fth_word(pForth);

	// find the word in the dictionary
	char *oldWordName = (char*)fth_pop(pForth);
	DictionaryEntry *pOldWord = fth_tick_internal(pForth, oldWordName);
	if (!pOldWord)
	{
		// old word not found!
		pForth->forth_print(oldWordName);
		pForth->forth_print(" word not found!\n");
		return FTH_FALSE;
	}

	// append the compilation semantics
	fth_write_to_cp(pForth, (ForthNumber)pOldWord);

	return FTH_TRUE;
}

// implements [']
// : ['] ( compilation: "name" --; run-time: -- xt ) ' POSTPONE literal ; immediate
static int fth_bracket_tick(ForthState *pForth)
{

	return FTH_TRUE;
}

// implements
//static int fth_xxx(ForthState *pForth)
//{
//
//	return FTH_TRUE;
//}

//
// Note: Since word lookup is currently O(n) on dictionary size
// try to keep common words towards the bottom for speed
//
static const ForthWordSet basic_lib[] =
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
//	{ "POSTPONE", fth_postpone },

	{ "BRANCH?", fth_conditional_branch },
	{ "BRANCH", fth_branch },
	{ "IF", fth_if },
	{ "THEN", fth_then },
	{ "ELSE", fth_else },

	{ ">BODY", fth_body },
	{ "I", fth_r_fetch },
	{ "J", fth_j },
	{ "R@", fth_r_fetch },
	{ "R>", fth_from_r },
	{ ">R", fth_to_r },
	{ "LOAD", fth_load },
	{ "EXECUTE", fth_execute },
	{ "MARKER", fth_marker },
	{ "INTERPRET", fth_quit},
	{ "NUMBER", fth_number },
	{ "'", fth_tick },
	{ ",", fth_comma },
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
	{ "LITERAL", fth_literal },

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
	"ELSE",
	"BEGIN",
	"AGAIN",
	"UNTIL",
	"WHILE",
	"REPEAT",
	"DO",
	"LOOP",
	"+LOOP",
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
// Public Forth functions
//========================

// allows embedders to set their own output function
int fth_set_output_function(ForthState *pForth, ForthOutputFunc f)
{
	pForth->forth_print = f;
	return FTH_TRUE;
}

// C API to create a new forth state
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

	// setup stacks
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

	pForth->hexmode = 0;
	pForth->maxr = 0;
	pForth->maxs = 0;

	pForth->BLK = stdin;

	// register built-in words
	fth_register_wordset(pForth, basic_lib);
	fth_register_core_wordset(pForth);

	// setup immediate words
	for (const char **word = immediate_words; *word; word++)
		fth_make_immediate(pForth, *word);

	// setup compile-only words
	for (const char **word = compile_only_words; *word; word++)
		fth_make_compile_only(pForth, *word);

	// setup defining words
	/* none yet! */

	// setup hidden words
	/* none yet! */

	// setup variables
	fth_define_word_var(pForth, "CP", (ForthNumber*)pForth->CP);
	fth_define_word_var(pForth, "RP", (ForthNumber*)pForth->RP);
	fth_define_word_var(pForth, "SP", (ForthNumber*)pForth->SP);
	fth_define_word_var(pForth, ">IN", (ForthNumber*)pForth->IN);
	fth_define_word_var(pForth, "TIB", (ForthNumber*)pForth->TIB);

	fth_define_word_var(pForth, "ENV.MAXS", (ForthNumber*)&pForth->maxs);
	fth_define_word_var(pForth, "ENV.MAXR", (ForthNumber*)&pForth->maxr);
	fth_define_word_var(pForth, "ENV.HEX", (ForthNumber*)&pForth->hexmode);

	// load the core library
	load_helper(pForth, "core.fth");

	return pForth;
}

// C API to cleanup and free forth state
int fth_delete_state(ForthState *pForth)
{
	free(pForth->dictionary_base);
	free(pForth);

	return FTH_TRUE;
}

// C API to mark the given word as hidden
int fth_make_hidden(ForthState* pForth, const char *word)
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

// C API to mark the given word as immediate
int fth_make_immediate(ForthState* pForth, const char *word)
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

// C API to mark the given word as compile-only
int fth_make_compile_only(ForthState* pForth, const char *word)
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
int fth_make_xt_required(ForthState* pForth, const char *word)
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

// C API to register a wordset
int fth_register_wordset(ForthState *pForth, const ForthWordSet words[])
{
	for (int i = 0; words[i].wordName && words[i].func; i++)
	{
		fth_make_dict_entry(pForth, words[i].wordName);
		pForth->head->code_pointer = words[i].func;
	}

	return FTH_TRUE;
}

// C API to push a value on the stack
int fth_push(ForthState *pForth, ForthNumber val)
{
	// check for stack overflow
	if ((pForth->SP - pForth->stack) + 1 > FTH_STACK_SIZE)
	{
		pForth->forth_print("Stack overflow!\n");
		return FTH_FALSE;
	}

	*pForth->SP++ = val;

	ForthNumber depth = pForth->SP - pForth->stack;
	if (depth > pForth->maxs)
		pForth->maxs = depth;

	return FTH_TRUE;
}

// C API to pop a value off the stack
ForthNumber fth_pop(ForthState *pForth)
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

// C API to return the TOS without removing it
int fth_peek(ForthState *pForth)
{
	if (pForth->SP == pForth->stack)
	{
		pForth->forth_print("Stack is empty!\n");
		return FTH_FALSE;
	}

	return *(pForth->SP-1);
}

// C API for defining user constants
int fth_define_word_const(ForthState *pForth, const char *name, ForthNumber val)
{
	fth_make_dict_entry(pForth, name);

	pForth->head->code_pointer = fth_const_imp;
	pForth->head->flags.xt_on_stack = 1;
	pForth->head->flags.constant = 1;

	// store constant value in dictionary
	fth_write_to_cp(pForth, val);

	return FTH_TRUE;
}

// C API for defining user variables
int fth_define_word_var(ForthState *pForth, const char *name, ForthNumber *pVar)
{
	fth_make_dict_entry(pForth, name);

	pForth->head->code_pointer = fth_user_var_imp;
	pForth->head->flags.xt_on_stack = 1;
	pForth->head->flags.variable = 1;

	// store var pointer in dictionary
	fth_write_to_cp(pForth, (ForthNumber)pVar);

	return FTH_TRUE;
}

// C API for the outer interpreter
int fth_update(ForthState *pForth)
{
	return fth_quit(pForth);
}
