#include <stdio.h>
#include <math.h>

#include "firth.h"

// implements BYTE
static FirthNumber fth_bye(FirthState *pFirth)
{
	pFirth->halted = 1;

	return FTH_TRUE;
}

// implements '.' - print out the TOS
static FirthNumber fth_dot(FirthState *pFirth)
{
	FirthNumber n = fth_pop(pFirth);

	if (pFirth->hexmode)
		firth_printf(pFirth, "0x%0X ", n);
	else
		firth_printf(pFirth, "%d ", n);

	return FTH_TRUE;
}

// implements WORDS
static FirthNumber fth_words(FirthState *pFirth)
{
	int count = 0;

	pFirth->firth_print("\nDictionary Listing...\n");

	DictionaryEntry *pIter = pFirth->head;

	while (pIter)
	{
		if (!pIter->flags.hidden)
		{
			firth_printf(pFirth, "%s ", pIter->name);

			// add word meta-data
			if (pIter->flags.colon_word)
				pFirth->firth_print("[colon word] ");
			else
				pFirth->firth_print("[native] ");
			if (pIter->flags.immediate)
				pFirth->firth_print("[immediate] ");
			if (pIter->flags.compile_only)
				pFirth->firth_print("[compile-only] ");
			if (pIter->flags.constant)
				pFirth->firth_print("[constant] ");
			if (pIter->flags.variable)
				pFirth->firth_print("[variable] ");

			pFirth->firth_print("\n");
			count++;

			// pause every page
			//if (!(count % 24))
			//{
			//	pFirth->firth_print("(more)\n");
			//	getc(stdin);
			//}
		}

		pIter = pIter->next;
	}

	firth_printf(pFirth, "\nThere are %d WORDS in the dictionary.\n", count);

	return FTH_TRUE;
}

//
static FirthNumber fth_emit(FirthState *pFirth)
{
	FirthNumber n = fth_pop(pFirth);
	firth_printf(pFirth, "%c", (char)n);
	return FTH_TRUE;
}

//
static FirthNumber fth_plus(FirthState *pFirth)
{
	FirthNumber b = fth_pop(pFirth);
	FirthNumber a = fth_pop(pFirth);
	return fth_push(pFirth, a + b);
}

//
static FirthNumber fth_minus(FirthState *pFirth)
{
	FirthNumber b = fth_pop(pFirth);
	FirthNumber a = fth_pop(pFirth);
	return fth_push(pFirth, a - b);
}

//
static FirthNumber fth_mul(FirthState *pFirth)
{
	FirthNumber b = fth_pop(pFirth);
	FirthNumber a = fth_pop(pFirth);
	return fth_push(pFirth, a * b);
}

//
static FirthNumber fth_div(FirthState *pFirth)
{
	FirthNumber b = fth_pop(pFirth);
	FirthNumber a = fth_pop(pFirth);
	return fth_push(pFirth, a / b);
}

// fetch value at addr on TOS
static FirthNumber fth_fetch(FirthState *pFirth)
{
	FirthNumber *pInt = (FirthNumber*)fth_pop(pFirth);
	return fth_push(pFirth, *pInt);
}

// ( n addr -- )
static FirthNumber fth_store(FirthState *pFirth)
{
	FirthNumber *pInt = (FirthNumber*)fth_pop(pFirth);
	FirthNumber val = fth_pop(pFirth);

	*pInt = val;

	return FTH_TRUE;
}

//
static FirthNumber fth_swap(FirthState *pFirth)
{
	FirthNumber a = fth_pop(pFirth);
	FirthNumber b = fth_pop(pFirth);
	
	fth_push(pFirth, a);
	
	return fth_push(pFirth, b);
}

// drop ( n -- )
static FirthNumber fth_drop(FirthState *pFirth)
{
	return fth_pop(pFirth);
}

// dup ( n -- n n )
static FirthNumber fth_dup(FirthState *pFirth)
{
	FirthNumber a = fth_pop(pFirth);
	fth_push(pFirth, a);

	return fth_push(pFirth, a);
}

// rot (n1 n2 n3 -- n2 n3 n1)
static FirthNumber fth_rot(FirthState *pFirth)
{
	FirthNumber n3 = fth_pop(pFirth);
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);

	fth_push(pFirth, n2);
	fth_push(pFirth, n3);

	return fth_push(pFirth, n1);
}

// over ( n1 n2 -- n1 n2 n1 )
static FirthNumber fth_over(FirthState *pFirth)
{
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);
	
	fth_push(pFirth, n1);
	fth_push(pFirth, n2);
	return fth_push(pFirth, n1);
}

//
static FirthNumber fth_equal(FirthState *pFirth)
{
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);
	return fth_push(pFirth, n1 == n2 ? FTH_TRUE : FTH_FALSE);
}

//
static FirthNumber fth_not_equal(FirthState *pFirth)
{
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);
	return fth_push(pFirth, n1 != n2 ? FTH_TRUE : FTH_FALSE);
}

//
static FirthNumber fth_greater_than(FirthState *pFirth)
{
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);
	return fth_push(pFirth, n1 > n2 ? FTH_TRUE : FTH_FALSE);
}

//
static FirthNumber fth_less_than(FirthState *pFirth)
{
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);
	return fth_push(pFirth, n1 < n2 ? FTH_TRUE : FTH_FALSE);
}

//
static FirthNumber fth_equal_zero(FirthState *pFirth)
{
	FirthNumber n = fth_pop(pFirth);
	return fth_push(pFirth, n == 0 ? FTH_TRUE : FTH_FALSE);
}

//
static FirthNumber fth_not_equal_zero(FirthState *pFirth)
{
	FirthNumber n = fth_pop(pFirth);
	return fth_push(pFirth, n != 0 ? FTH_TRUE : FTH_FALSE);
}

//
static FirthNumber fth_less_than_zero(FirthState *pFirth)
{
	FirthNumber n = fth_pop(pFirth);
	return fth_push(pFirth, n < 0 ? FTH_TRUE : FTH_FALSE);
}


static FirthNumber fth_greater_than_zero(FirthState *pFirth)
{
	FirthNumber n = fth_pop(pFirth);
	return fth_push(pFirth, n > 0 ? FTH_TRUE : FTH_FALSE);
}

//
static FirthNumber fth_and(FirthState *pFirth)
{
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);
	return fth_push(pFirth, n1 & n2);
}

//
static FirthNumber fth_or(FirthState *pFirth)
{
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);
	return fth_push(pFirth, n1 | n2);
}

//
static FirthNumber fth_not(FirthState *pFirth)
{
	FirthNumber n = fth_pop(pFirth);
	return fth_push(pFirth, !n);
}

//
static FirthNumber fth_xor(FirthState *pFirth)
{
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);
	return fth_push(pFirth, n1 ^ n2);
}

//
static FirthNumber fth_mod(FirthState *pFirth)
{
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);
	return fth_push(pFirth, n1 % n2);
}

//
static FirthNumber fth_div_mod(FirthState *pFirth)
{
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);

	fth_push(pFirth, n1 % n2); // push rem
	return fth_push(pFirth, n1 / n2); // push quotient
}

//
static FirthNumber fth_mul_div(FirthState *pFirth)
{
	FirthNumber n3 = fth_pop(pFirth);
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);

	long long n4 = (long long)(n1) * n2;
	return fth_push(pFirth, (FirthNumber)(n4 / n3));
}

//
static FirthNumber fth_lshift(FirthState *pFirth)
{
	FirthNumber shift = fth_pop(pFirth);
	FirthNumber num = fth_pop(pFirth);
	return fth_push(pFirth, num << shift);
}

//
static FirthNumber fth_rshift(FirthState *pFirth)
{
	FirthNumber shift = fth_pop(pFirth);
	FirthNumber num = fth_pop(pFirth);
	return fth_push(pFirth, num >> shift);
}

//
static FirthNumber fth_pow(FirthState *pFirth)
{
	FirthNumber n2 = fth_pop(pFirth);
	FirthNumber n1 = fth_pop(pFirth);
	return fth_push(pFirth, (FirthNumber)(pow(n1, n2) + 0.5f));
}

//
//static FirthNumber fth_xxx(FirthState *pFirth)
//{
//	FirthNumber n = fth_pop(pFirth);
//
//}

// register our collection of custom words
static const FirthWordSet core_lib[] =
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
	{ "/MOD", fth_div_mod },
	{ "*/", fth_mul_div },
	{ "POW", fth_pow },

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

	{ "LSHIFT", fth_lshift },
	{ "RSHIFT", fth_rshift },

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
FirthNumber fth_register_core_wordset(FirthState *pFirth)
{
	return fth_register_wordset(pFirth, core_lib);
}