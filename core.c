#include <stdio.h>

#include "forth.h"

// implements BYTE
static int fth_bye(ForthState *pForth)
{
	pForth->halted = 1;

	return FTH_TRUE;
}

// implements '.' - print out the TOS
static int fth_dot(ForthState *pForth)
{
	ForthNumber n = fth_pop(pForth);

	forth_printf(pForth, "%d ", n);
	return FTH_TRUE;
}

// implements WORDS
static int fth_words(ForthState *pForth)
{
	int count = 0;

	pForth->forth_print("\nDictionary Listing...\n");

	DictionaryEntry *pIter = pForth->head;

	while (pIter)
	{
		if (!pIter->flags.hidden)
		{
			forth_printf(pForth, "%s ", pIter->name);

			// add word meta-data
			if (pIter->flags.immediate)
				pForth->forth_print("[immediate] ");
			if (pIter->flags.colon_word)
				pForth->forth_print("[colon word] ");
			if (pIter->flags.compile_only)
				pForth->forth_print("[compile only] ");

			pForth->forth_print("\n");
			count++;
		}

		pIter = pIter->next;
	}

	forth_printf(pForth, "\nThere are %d WORDS in the dictionary.\n", count);

	return FTH_TRUE;
}

//
static int fth_emit(ForthState *pForth)
{
	ForthNumber n = fth_pop(pForth);
	forth_printf(pForth, "%c", (char)n);
	return FTH_TRUE;
}

//
static int fth_plus(ForthState *pForth)
{
	ForthNumber b = fth_pop(pForth);
	ForthNumber a = fth_pop(pForth);
	return fth_push(pForth, a + b);
}

//
static int fth_minus(ForthState *pForth)
{
	ForthNumber b = fth_pop(pForth);
	ForthNumber a = fth_pop(pForth);
	return fth_push(pForth, a - b);
}

//
static int fth_mul(ForthState *pForth)
{
	ForthNumber b = fth_pop(pForth);
	ForthNumber a = fth_pop(pForth);
	return fth_push(pForth, a * b);
}

//
static int fth_div(ForthState *pForth)
{
	ForthNumber b = fth_pop(pForth);
	ForthNumber a = fth_pop(pForth);
	return fth_push(pForth, a / b);
}

// fetch value at addr on TOS
static int fth_fetch(ForthState *pForth)
{
	ForthNumber *pInt = (ForthNumber*)fth_pop(pForth);
	return fth_push(pForth, *pInt);
}

// ( n addr -- )
static int fth_store(ForthState *pForth)
{
	ForthNumber *pInt = (ForthNumber*)fth_pop(pForth);
	ForthNumber val = fth_pop(pForth);

	*pInt = val;

	return FTH_TRUE;
}

//
static int fth_swap(ForthState *pForth)
{
	ForthNumber a = fth_pop(pForth);
	ForthNumber b = fth_pop(pForth);
	
	fth_push(pForth, a);
	
	return fth_push(pForth, b);
}

// drop ( n -- )
static int fth_drop(ForthState *pForth)
{
	return fth_pop(pForth);
}

// dup ( n -- n n )
static int fth_dup(ForthState *pForth)
{
	ForthNumber a = fth_pop(pForth);
	fth_push(pForth, a);

	return fth_push(pForth, a);
}

// rot (n1 n2 n3 -- n2 n3 n1)
static int fth_rot(ForthState *pForth)
{
	ForthNumber n3 = fth_pop(pForth);
	ForthNumber n2 = fth_pop(pForth);
	ForthNumber n1 = fth_pop(pForth);

	fth_push(pForth, n2);
	fth_push(pForth, n3);

	return fth_push(pForth, n1);
}

// over ( n1 n2 -- n1 n2 n1 )
static int fth_over(ForthState *pForth)
{
	ForthNumber n2 = fth_pop(pForth);
	ForthNumber n1 = fth_pop(pForth);
	
	fth_push(pForth, n1);
	fth_push(pForth, n2);
	return fth_push(pForth, n1);
}

//
static int fth_equal(ForthState *pForth)
{
	ForthNumber n2 = fth_pop(pForth);
	ForthNumber n1 = fth_pop(pForth);
	return fth_push(pForth, n1 == n2 ? FTH_TRUE : FTH_FALSE);
}

//
static int fth_not_equal(ForthState *pForth)
{
	ForthNumber n2 = fth_pop(pForth);
	ForthNumber n1 = fth_pop(pForth);
	return fth_push(pForth, n1 != n2 ? FTH_TRUE : FTH_FALSE);
}

//
static int fth_greater_than(ForthState *pForth)
{
	ForthNumber n2 = fth_pop(pForth);
	ForthNumber n1 = fth_pop(pForth);
	return fth_push(pForth, n1 > n2 ? FTH_TRUE : FTH_FALSE);
}

//
static int fth_less_than(ForthState *pForth)
{
	ForthNumber n2 = fth_pop(pForth);
	ForthNumber n1 = fth_pop(pForth);
	return fth_push(pForth, n1 < n2 ? FTH_TRUE : FTH_FALSE);
}

//
static int fth_equal_zero(ForthState *pForth)
{
	ForthNumber n = fth_pop(pForth);
	return fth_push(pForth, n == 0 ? FTH_TRUE : FTH_FALSE);
}

//
static int fth_not_equal_zero(ForthState *pForth)
{
	ForthNumber n = fth_pop(pForth);
	return fth_push(pForth, n != 0 ? FTH_TRUE : FTH_FALSE);
}

//
static int fth_less_than_zero(ForthState *pForth)
{
	ForthNumber n = fth_pop(pForth);
	return fth_push(pForth, n < 0 ? FTH_TRUE : FTH_FALSE);
}


static int fth_greater_than_zero(ForthState *pForth)
{
	ForthNumber n = fth_pop(pForth);
	return fth_push(pForth, n > 0 ? FTH_TRUE : FTH_FALSE);
}

//
static int fth_and(ForthState *pForth)
{
	ForthNumber n2 = fth_pop(pForth);
	ForthNumber n1 = fth_pop(pForth);
	return fth_push(pForth, n1 & n2);
}

//
static int fth_or(ForthState *pForth)
{
	ForthNumber n2 = fth_pop(pForth);
	ForthNumber n1 = fth_pop(pForth);
	return fth_push(pForth, n1 | n2);
}

//
static int fth_not(ForthState *pForth)
{
	ForthNumber n = fth_pop(pForth);
	return fth_push(pForth, !n);
}

//
static int fth_xor(ForthState *pForth)
{
	ForthNumber n2 = fth_pop(pForth);
	ForthNumber n1 = fth_pop(pForth);
	return fth_push(pForth, n1 ^ n2);
}

//
static int fth_mod(ForthState *pForth)
{
	ForthNumber n2 = fth_pop(pForth);
	ForthNumber n1 = fth_pop(pForth);
	return fth_push(pForth, n1 % n2);
}

//
//static int fth_xxx(ForthState *pForth)
//{
//	ForthNumber n = fth_pop(pForth);
//
//}

// register our collection of custom words
static const ForthWordSet core_lib[] =
{
	{ "BYE", fth_bye },
	{ ".", fth_dot },
	{ "WORDS", fth_words },
	{ "EMIT",  fth_emit },

	// math
	{ "+",  fth_plus },
	{ "-",  fth_minus },
	{ "*",  fth_mul },
	{ "/",  fth_div },

	{ "MOD", fth_mod },

	// relational
	{ "=", fth_equal },
	{ "<>", fth_not_equal },
	{ ">", fth_greater_than },
	{ "<", fth_less_than },

	{ "0=", fth_equal_zero },
	{ "0<>", fth_not_equal_zero },
	{ "0>", fth_greater_than_zero },
	{ "0<", fth_less_than_zero },

	// bitwise
	{ "AND", fth_and },
	{ "OR", fth_or },
	{ "NOT", fth_not },
	{ "XOR", fth_xor },

	// variables
	{ "@", fth_fetch },
	{ "!", fth_store },

	// stack ops
	{ "SWAP", fth_swap },
	{ "DROP", fth_drop },
	{ "DUP", fth_dup },
	{ "ROT", fth_rot },
	{ "OVER", fth_over },

	{ NULL, NULL }
};

//
int fth_register_core_wordset(ForthState *pForth)
{
	return fth_register_wordset(pForth, core_lib);
}