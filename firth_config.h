#pragma once

// value to set for uninitialized variables
// redefine this to a value that makes sense for your platform
#ifndef FTH_UNINITIALIZED
#	define FTH_UNINITIALIZED 0
#endif

// Firth is case insensitive by default
#ifndef FTH_CASE_SENSITIVE
#	define FTH_CASE_SENSITIVE 0
#endif

#ifndef FTH_DIR_SEPARATOR
	#ifdef WIN32
	#	define FTH_DIR_SEPARATOR '\\'
	#else
	#	define FTH_DIR_SEPARATOR '/'
	#endif
#endif

#ifndef FTH_MAX_PATH
	#ifdef _MAX_PATH
	#	define FTH_MAX_PATH _MAX_PATH
	#else
	#	define FTH_MAX_PATH 260
	#endif
#endif

// Adjust this to increase the max dictionary size
#define FTH_ENV_SIZE 16 * 1024

// Adjust this to change the default stack size
#define FTH_STACK_SIZE 512

// Adjust this to change the default return stack size
#define FTH_RETURN_STACK_SIZE 512

// Set this to zero to remove all float support
#ifndef FTH_INCLUDE_FLOAT
#	define FTH_INCLUDE_FLOAT 1
#endif

// Sets the limit for the length of a word name
#define FTH_MAX_WORD_NAME 256

// Sets the limit for the length of a firth_printf result
#define FTH_MAX_PRINTF_SIZE 512

// Defines the underlying type for a Number
typedef int FirthNumber;

// Defines the underlying type for a float Number
#if FTH_INCLUDE_FLOAT == 1
	typedef float FirthFloat;

	// Defines epsilon for floating point precision
#	define FTH_EPSILON 1e-5f
#endif

// Enables Firth modern syntax
#ifndef FTH_FIRTH_SYNTAX
#	define FTH_FIRTH_SYNTAX 1
#endif

#if FTH_FIRTH_SYNTAX == 1
	// Enable fn as colon synonym
	#ifndef FTH_I_LIKE_RUST
		#define FTH_I_LIKE_RUST 1
	#endif

	// Enable func as colon synonym
	#ifndef FTH_I_LIKE_SWIFT
		#define FTH_I_LIKE_SWIFT 1
	#endif

	// Enable function as colon synonym
	#ifndef FTH_I_LIKE_JAVASCRIPT
		#define FTH_I_LIKE_JAVASCRIPT 1
	#endif

	// Enable def as colon synonym
	#ifndef FTH_I_LIKE_PYTHON
		#define FTH_I_LIKE_PYTHON 1
	#endif

#endif
