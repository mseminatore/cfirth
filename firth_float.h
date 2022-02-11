#pragma once

#if FTH_INCLUDE_FLOAT == 1
	int firth_register_float(FirthState *pFirth);
	int fth_define_word_fconst(FirthState *pFirth, const char *name, FirthFloat val);
	int fth_define_word_fvar(FirthState *pFirth, const char *name, FirthFloat *pVar);
	FirthFloat fth_popf(FirthState *pFirth);
	int fth_pushf(FirthState *pFirth, FirthFloat val);
	FirthFloat fth_peekf(FirthState *pFirth);
#endif
