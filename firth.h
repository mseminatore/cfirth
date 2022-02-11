#pragma once

#include <stdio.h>
#include <math.h>
#include "firth_config.h"

//
#define FTH_TRUE -1
#define FTH_FALSE 0

//
typedef unsigned char BYTE;
typedef unsigned char bool;
extern bool true;
extern bool false;

// type for custom output functions
typedef void(*FirthOutputFunc)(char*);

//
typedef struct FirthState FirthState;
typedef int (*FirthFunc)(FirthState *pFirth);
typedef struct DictionaryEntry DictionaryEntry;

//
typedef struct
{
	unsigned int immediate : 1;
	unsigned int hidden : 1;
	unsigned int compile_only : 1;
	unsigned int colon_word : 1;
	unsigned int xt_on_stack : 1;
	unsigned int constant : 1;
	unsigned int variable : 1;
	unsigned int unused : 25;
} WordFlags;

//
#pragma pack(push, 1)
struct DictionaryEntry
{
	DictionaryEntry *next;
	FirthFunc code_pointer;
	WordFlags flags;
	unsigned char name_len;
	char name[];
};
#pragma pack(pop)

//
struct FirthState
{
	FirthNumber *IP;				// interpreter pointer
	BYTE *CP;				// dictionary pointer
	DictionaryEntry *head;	// head of dictionary linked list
	BYTE *dictionary_base;	// beginning of dictionary memory

	FILE *BLK;

	FirthNumber hexmode;
	FirthNumber maxr;
	FirthNumber maxs;

	FirthNumber stack[FTH_STACK_SIZE];	// bottom of stack
	FirthNumber *SP;					// top of stack pointer

	FirthNumber return_stack[FTH_RETURN_STACK_SIZE];
	FirthNumber *RP;

#if FTH_INCLUDE_FLOAT == 1
	FirthFloat float_stack[FTH_STACK_SIZE];
	FirthFloat *FP;
	FirthNumber maxf;
#endif

	char TIB[FTH_MAX_WORD_NAME];	// text input buffer
	char *IN;				// input pointer
	FirthNumber tib_len;	// length of input

	FirthOutputFunc firth_print;
	BYTE halted;
	bool compiling;
};

// custom function registration struct
typedef struct
{
	char *wordName;
	FirthFunc func;
} FirthWordSet;

// Public Firth functions
FirthState *fth_create_state();
int fth_delete_state(FirthState*);

int fth_tick(FirthState *pFirth);
int fth_word(FirthState *pFirth);
int fth_create(FirthState *pFirth);

int fth_push(FirthState *pFirth, FirthNumber val);
FirthNumber fth_pop(FirthState *pFirth);
int fth_peek(FirthState *pFirth);

int fth_set_output_function(FirthState *pFirth, FirthOutputFunc f);
void firth_printf(FirthState *pFirth, char *format, ...);

//int fth_interpret(FirthState *pFirth);
int fth_quit(FirthState *pFirth);

int fth_register_wordset(FirthState *pFirth, const FirthWordSet words[]);
int fth_register_core_wordset(FirthState *pFirth);

int fth_make_compile_only(FirthState *pFirth, const char *word);
int fth_make_immediate(FirthState *pFirth, const char *word);
int fth_make_hidden(FirthState *pFirth, const char *word);
int fth_make_xt_required(FirthState *pFirth, const char *word);

int fth_define_word_const(FirthState*, const char *name, FirthNumber val);
int fth_define_word_var(FirthState*, const char *name, FirthNumber *pVar);

int fth_update(FirthState *pFirth);
