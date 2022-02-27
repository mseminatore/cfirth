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
typedef FirthNumber (*FirthFunc)(FirthState *pFirth);
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

typedef struct
{
	FILE *fd;
	char dirname[FTH_MAX_PATH];
} InputNode;

//
struct FirthState
{
	FirthNumber *IP;				// interpreter pointer
	BYTE *CP;				// dictionary pointer
	DictionaryEntry *head;	// head of dictionary linked list
	BYTE *dictionary_base;	// beginning of dictionary memory

	FILE *BLK;
	InputNode input_stack[FTH_INPUT_STACK_SIZE];
	FirthNumber ISP;	// input stack pointer

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
	char *INP;				// input pointer
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
FirthNumber fth_delete_state(FirthState*);

FirthNumber fth_push(FirthState *pFirth, FirthNumber val);
FirthNumber fth_pop(FirthState *pFirth);
FirthNumber fth_peek(FirthState *pFirth);

FirthNumber fth_set_output_function(FirthState *pFirth, FirthOutputFunc f);
void firth_printf(FirthState *pFirth, char *format, ...);

FirthNumber fth_register_wordset(FirthState *pFirth, const FirthWordSet words[]);
FirthNumber fth_register_core_wordset(FirthState *pFirth);

FirthNumber fth_make_compile_only(FirthState *pFirth, const char *word);
FirthNumber fth_make_immediate(FirthState *pFirth, const char *word);
FirthNumber fth_make_hidden(FirthState *pFirth, const char *word);
FirthNumber fth_make_xt_required(FirthState *pFirth, const char *word);

FirthNumber fth_define_word_const(FirthState*, const char *name, FirthNumber val);
FirthNumber fth_define_word_var(FirthState*, const char *name, FirthNumber *pVar);

FirthNumber fth_update(FirthState *pFirth);

FirthNumber fth_parse_string(FirthState *pFirth, const char *str);
FirthNumber fth_exec_word(FirthState *pFirth, const char *str);
FirthNumber fth_exec_word1(FirthState *pFirth, const char *word, FirthNumber n);
FirthNumber fth_exec_word2(FirthState *pFirth, const char *word, FirthNumber n1, FirthNumber n2);
FirthNumber fth_exec_word3(FirthState *pFirth, const char *word, FirthNumber n1, FirthNumber n2, FirthNumber n3);
FirthNumber *fth_get_var(FirthState *pFirth, const char *word);
