#pragma once

//
#define FTH_TRUE -1
#define FTH_FALSE 0

#define FTH_ENV_SIZE 4096
#define FTH_STACK_SIZE 512
#define FTH_RETURN_STACK_SIZE 512

//
#define FTH_MAX_WORD 256

// Sets the limit for the length of a firth_printf result
#define FTH_MAX_PRINTF_SIZE 512

// value to set for uninitialized variables
#define FTH_UNINITIALIZED 0

typedef int ForthNumber;

//
typedef unsigned char BYTE;
typedef unsigned char bool;
extern bool true;
extern bool false;

// type for custom output functions
typedef void(*ForthOutputFunc)(char*);

//
typedef struct ForthState ForthState;
typedef int (*ForthFunc)(ForthState *pForth);
typedef struct DictionaryEntry DictionaryEntry;

//
typedef struct
{
	unsigned int immediate : 1;
	unsigned int hidden : 1;
	unsigned int compile_only : 1;
	unsigned int colon_word : 1;
	unsigned int xt_on_stack : 1;
	unsigned int unused : 27;
} WordFlags;

//
#pragma pack(push, 1)
struct DictionaryEntry
{
	DictionaryEntry *next;
	ForthFunc code_pointer;
	WordFlags flags;
	unsigned char name_len;
	char name[];
};
#pragma pack(pop)

//
struct ForthState
{
	ForthNumber *IP;				// interpreter pointer
	BYTE *CP;				// dictionary pointer
	DictionaryEntry *head;	// head of dictionary linked list
	BYTE *dictionary_base;	// beginning of dictionary memory

	FILE *BLK;

	ForthNumber hexmode;
	ForthNumber maxr;
	ForthNumber maxs;

	ForthNumber *stack;		// bottom of stack
	ForthNumber *SP;		// top of stack pointer

	ForthNumber *return_stack;
	ForthNumber *RP;

	char TIB[FTH_MAX_WORD];	// text input buffer
	char *IN;				// input pointer
	ForthNumber tib_len;	// length of input

	ForthOutputFunc forth_print;
	BYTE halted;
	bool compiling;
};

// custom function registration struct
typedef struct
{
	char *wordName;
	ForthFunc func;
} ForthWordSet;

// Public Forth functions
ForthState *fth_create_state();
int fth_delete_state(ForthState*);

int fth_tick(ForthState *pForth);
int fth_word(ForthState *pForth);
int fth_create(ForthState *pForth);

int fth_push(ForthState *pForth, ForthNumber val);
ForthNumber fth_pop(ForthState *pForth);
int fth_peek(ForthState *pForth);

int fth_set_output_function(ForthState *pForth, ForthOutputFunc f);
void forth_printf(ForthState *pForth, char *format, ...);

//int fth_interpret(ForthState *pForth);
int fth_quit(ForthState *pForth);

int fth_register_wordset(ForthState *pForth, const ForthWordSet words[]);
static int fth_xxx(ForthState * pForth);
int fth_register_core_wordset(ForthState *pForth);

int fth_make_compile_only(ForthState* pForth, char *word);
int fth_make_immediate(ForthState* pForth, char *word);
int fth_make_hidden(ForthState* pForth, char *word);
int fth_make_xt_required(ForthState* pForth, char *word);

int fth_define_word_const(ForthState*, const char *name, ForthNumber val);
int fth_define_word_var(ForthState*, const char *name, ForthNumber *pVar);
