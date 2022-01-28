#pragma once

//
#define FTH_TRUE -1
#define FTH_FALSE 0

#define FTH_ENV_SIZE 4096
#define FTH_STACK_SIZE 1024

//
#define FTH_MAX_WORD 256

// Sets the limit for the length of a firth_printf result
#define FTH_MAX_PRINTF_SIZE 512

//
typedef unsigned char BYTE;
typedef unsigned char bool;
bool true = ~0;
bool false = 0;

// type for custom output functions
typedef void(*ForthOutputFunc)(char*);

//
typedef struct ForthState ForthState;
typedef int (*ForthFunc)(ForthState *pForth);
typedef struct DictionaryEntry DictionaryEntry;

//
#pragma pack(1)
struct DictionaryEntry
{
	DictionaryEntry *next;
	ForthFunc code_pointer;
	unsigned char name_len;
	char name[];
};

//
struct ForthState
{
	BYTE *IP;
	BYTE *CP;
	DictionaryEntry *head;
	BYTE *dictionary_base;

	int *stack;
	int *TOS;
	char TIB[FTH_MAX_WORD];
	char *IN;

	ForthOutputFunc forth_print;
	DictionaryEntry *current_word;
	BYTE halted;
	bool compiling;
};

// custom function registration struct
typedef struct
{
	char *wordName;
	ForthFunc func;
} ForthWordSet;

//
ForthState *fth_create_state();
int fth_delete_state(ForthState*);

int fth_tick(ForthState *pForth);
int fth_word(ForthState *pForth);
int fth_create(ForthState *pForth);

int fth_push(ForthState *pForth, int val);
int fth_pop(ForthState *pForth);
int fth_set_output_function(ForthState *pForth, ForthOutputFunc f);

int fth_register_wordset(ForthState *pForth, const ForthWordSet words[]);
int fth_interpret(ForthState *pForth);

int fth_register_core_wordset(ForthState *pForth);
void forth_printf(ForthState *pForth, char *format, ...);
