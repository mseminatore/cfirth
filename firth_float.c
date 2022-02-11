#include "firth.h"
#include "firth_float.h"

//
FirthFloat fth_popf(FirthState *pFirth)
{
	// check for stack underflow
	if (pFirth->FP <= pFirth->float_stack)
	{
		pFirth->firth_print("Float Stack underflow!\n");
		return 0.0f;
	}

	pFirth->FP--;
	return *pFirth->FP;
}

//
int fth_pushf(FirthState *pFirth, FirthFloat val)
{
	// check for stack overflow
	if ((pFirth->FP - pFirth->float_stack) + 1 > FTH_STACK_SIZE)
	{
		pFirth->firth_print("Float Stack overflow!\n");
		return FTH_FALSE;
	}

	*pFirth->FP++ = val;

	FirthNumber depth = pFirth->FP - pFirth->float_stack;
	if (depth > pFirth->maxf)
		pFirth->maxf = depth;

	return FTH_TRUE;
}

//
FirthFloat fth_peekf(FirthState *pFirth)
{
	return *(pFirth->FP-1);
}

//
static int fadd(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	FirthFloat b = fth_popf(pFirth);
	fth_pushf(pFirth, a + b);

	return FTH_TRUE;
}

static int fsub(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	FirthFloat b = fth_popf(pFirth);
	fth_pushf(pFirth, a - b);

	return FTH_TRUE;
}

//
static int fmul(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	FirthFloat b = fth_popf(pFirth);
	fth_pushf(pFirth, a * b);

	return FTH_TRUE;
}

static int fdiv(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	FirthFloat b = fth_popf(pFirth);
	fth_pushf(pFirth, a / b);

	return FTH_TRUE;
}

static int fprint(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	firth_printf(pFirth, "%f", a);

	return FTH_TRUE;
}

static int fsin(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	fth_pushf(pFirth, (FirthFloat)sin(a));

	return FTH_TRUE;
}

static int fcos(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	fth_pushf(pFirth, (FirthFloat)cos(a));

	return FTH_TRUE;
}

static int ftan(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	fth_pushf(pFirth, (FirthFloat)tan(a));

	return FTH_TRUE;
}

static int fln(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	fth_pushf(pFirth, (FirthFloat)log(a));

	return FTH_TRUE;
}

static int fexp(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	fth_pushf(pFirth, (FirthFloat)exp(a));

	return FTH_TRUE;
}

static int fabsolute(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	fth_pushf(pFirth, (FirthFloat)fabs(a));

	return FTH_TRUE;
}

static int fsqrt(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	fth_pushf(pFirth, (FirthFloat)sqrt(a));

	return FTH_TRUE;
}

static int ffetch(FirthState *pFirth)
{
	FirthFloat *pNum = (FirthFloat*)fth_pop(pFirth);
	FirthFloat val = *pNum;
	fth_pushf(pFirth, val);

	return FTH_TRUE;
}

static int fstore(FirthState *pFirth)
{
	FirthFloat *pNum = (FirthFloat*)fth_pop(pFirth);
	FirthFloat val = fth_popf(pFirth);
	*pNum = val;

	return FTH_TRUE;
}

static int fdepth(FirthState *pFirth)
{
	fth_push(pFirth, (pFirth->FP - pFirth->float_stack));

	return FTH_TRUE;
}

static int fdup(FirthState *pFirth)
{
	FirthFloat a = fth_peekf(pFirth);
	fth_pushf(pFirth, a);

	return FTH_TRUE;
}

static int fdrop(FirthState *pFirth)
{
	fth_popf(pFirth);

	return FTH_TRUE;
}

static int fswap(FirthState *pFirth)
{
	FirthFloat a = fth_popf(pFirth);
	FirthFloat b = fth_popf(pFirth);
	
	fth_pushf(pFirth, a);
	fth_pushf(pFirth, b);

	return FTH_TRUE;
}

//
static int dotf(FirthState *pFirth)
{
	// make a copy of the stack
	FirthFloat *fs = pFirth->FP;

	pFirth->firth_print("Float Stack Top -> [ ");
	while (fs-- != pFirth->float_stack)
	{
		firth_printf(pFirth, "%f ", *fs);
	}

	pFirth->firth_print("]\n");

	return FTH_TRUE;
}

static int frot(FirthState *pFirth)
{
	FirthFloat n3 = fth_popf(pFirth);
	FirthFloat n2 = fth_popf(pFirth);
	FirthFloat n1 = fth_popf(pFirth);
	fth_pushf(pFirth, n2);
	fth_pushf(pFirth, n3);
	fth_pushf(pFirth, n1);

	return FTH_TRUE;
}

static int fover(FirthState *pFirth)
{
	FirthFloat n2 = fth_popf(pFirth);
	FirthFloat n1 = fth_popf(pFirth);
	fth_pushf(pFirth, n1);
	fth_pushf(pFirth, n2);
	fth_pushf(pFirth, n1);

	return FTH_TRUE;
}

static int fnip(FirthState *pFirth)
{
	FirthFloat n2 = fth_popf(pFirth);
	FirthFloat n1 = fth_popf(pFirth);
	fth_pushf(pFirth, n2);

	return FTH_TRUE;
}

static int ftuck(FirthState *pFirth)
{
	FirthFloat n2 = fth_popf(pFirth);
	FirthFloat n1 = fth_popf(pFirth);
	fth_pushf(pFirth, n2);
	fth_pushf(pFirth, n1);
	fth_pushf(pFirth, n2);

	return FTH_TRUE;
}

int feq(FirthState *pFirth)
{
	FirthFloat n2 = fth_popf(pFirth);
	FirthFloat n1 = fth_popf(pFirth);
	fth_push(pFirth, n1 == n2 ? FTH_TRUE : FTH_FALSE);

	return FTH_TRUE;
}

static int fless(FirthState *pFirth)
{
	FirthFloat n2 = fth_popf(pFirth);
	FirthFloat n1 = fth_popf(pFirth);
	fth_push(pFirth, n1 < n2 ? FTH_TRUE : FTH_FALSE);

	return FTH_TRUE;
}

static int fgreater(FirthState *pFirth)
{
	FirthFloat n2 = fth_popf(pFirth);
	FirthFloat n1 = fth_popf(pFirth);
	fth_push(pFirth, n1 > n2 ? FTH_TRUE : FTH_FALSE);

	return FTH_TRUE;
}

static int fne(FirthState *pFirth)
{
	FirthFloat n2 = fth_popf(pFirth);
	FirthFloat n1 = fth_popf(pFirth);
	fth_push(pFirth, n1 != n2 ? FTH_TRUE : FTH_FALSE);

	return FTH_TRUE;
}

// register our collection of custom words
static const FirthWordSet float_lib[] =
{
	{ "F+", fadd },
	{ "F-", fsub },
	{ "F*", fmul },
	{ "F/", fdiv },
	{ "F.", fprint },
	{ "SIN", fsin},
	{ "COS", fcos },
	{ "TAN", ftan },
	{ "LN", fln },
	{ "EXP", fexp },
	{ "FABS", fabsolute },
	{ "SQRT", fsqrt },
	{ "F@", ffetch },
	{ "F!", fstore },
	{ "FDEPTH", fdepth },
	{ "FDUP", fdup },
	{ "FDROP", fdrop },
	{ "FSWAP", fswap },
	{ ".F", dotf },
	{ "FROT", frot },
	{ "FOVER", fover },
	{ "FNIP", fnip },
	{ "FTUCK", ftuck },
	
	{ "F=", feq },
	{ "F<", fless },
	{ "F>", fgreater },
	{ "F<>", fne },

	{ NULL, NULL }
};

//
int firth_register_float(FirthState *pFirth)
{
	// common constants
	//fth_define_word_fconst("PI",	3.1415926f);
	//fth_define_word_fconst("PI_2",	1.5707963f);
	//fth_define_word_fconst("PI_4", 0.78539816f);

	fth_define_word_fconst(pFirth, "FEPSILON", FTH_EPSILON);

	return fth_register_wordset(pFirth, float_lib);
}
