# Copilot Instructions for C Firth

## Project Overview

Firth is an embeddable Forth-like scripting language written in portable C. Key design goals: zero heap allocations after startup (single pre-allocated memory block), small binary size (~37 KB release), and a Lua-inspired embedding API.

## Build & Test

**Build with Make:**
```sh
make          # builds the firth executable
make check    # runs the full test suite (loads test/core-tests.fth)
```

**Build with CMake:**
```sh
cmake -B build && cmake --build build
```

**Run a single test interactively:**
```sh
./firth
Firth> include test/test.fth
Firth> include test/core-tests.fth
```

**Run a specific test case inline:**
```sh
./firth
Firth> include test/test.fth
Firth> func my-test 2 3 + ;  5 assert-equals
```

## Architecture

The interpreter is built around a linked-list dictionary and subroutine threading (C function pointers—no bytecode):

- **`firth.h`** – Public embedding API. Defines `FirthState`/`Firth` struct, stack ops (`fth_push`, `fth_pop`), word registration (`fth_define_word`, `fth_define_word_const`, etc.).
- **`firth.c`** – Core engine: parser, compiler, REPL (`fth_update`), dictionary lookup, execution loop.
- **`core.c`** – Native C implementations of built-in words (arithmetic, stack manipulation, bitwise ops, control flow).
- **`firth_float.c/.h`** – Optional floating-point word set. Included when `FTH_INCLUDE_FLOAT=1` in `firth_config.h`.
- **`core.fth`** – Standard library words defined in Firth itself (e.g., `NIP`, `2DUP`, `MIN`, `MAX`, `TUCK`).
- **`main.c`** – Reference REPL host; also demonstrates the embedding API pattern.
- **`firth_config.h`** – Compile-time configuration (stack sizes, float support, case sensitivity, segment sizes).

**Memory model:** A single `malloc` at `fth_create_state()` allocates the entire runtime: dictionary, data stack, return stack, float stack, and TIB. Nothing is heap-allocated during parsing or execution.

**Dictionary structure:** Singly-linked list of `DictionaryEntry` nodes, searched from newest to oldest. Each entry holds a name, flags (immediate, hidden, compile-only, colon-word, constant, variable), a C function pointer, and optional body data.

**Dual stacks:** data stack (`stack[]`) and return stack (`return_stack[]`). An optional third float stack exists when float support is enabled.

## Key Conventions

**C naming:**
- All public API functions use the `fth_` prefix (e.g., `fth_push`, `fth_pop`, `fth_parse_string`, `fth_exec_word`).
- Native word handler functions match the signature `int word_name(Firth *pFirth)` and must return `FTH_TRUE` on success.

**Firth word naming:**
- Built-in words are UPPERCASE (`DUP`, `SWAP`, `DROP`).
- Firth adds modern lowercase synonyms: `func` (`:` ), `var` (`VARIABLE`), `const` (`CONSTANT`), `endif` (`THEN`), `for` (`DO`), `alloc` (`ALLOT`). Both forms work.

**Registering a new native word:**
```c
static int myWord(Firth *pFirth) {
    FirthNumber n = fth_pop(pFirth);
    fth_push(pFirth, n * 2);
    return FTH_TRUE;
}

// register individually or via a FirthWordSet table:
static const FirthWordSet myWords[] = {
    { "double", myWord },
    { NULL, NULL }
};
fth_register_wordset(pFirth, myWords);
```

**Adding a new built-in word:** implement in `core.c`, register in the init function there, and add tests to `test/core-tests.fth`. Document stack effects as `( n1 n2 -- n3 )`.

**Float words:** prefixed with `F` (e.g., `F+`, `FDUP`, `FSWAP`). Operate on the separate float stack. When documenting stack effects, use `( f: ... )` for float stack and `( s: ... )` for the data stack.

**Configuration:** Edit `firth_config.h` and rebuild to change `FTH_STACK_SIZE`, `FTH_RETURN_STACK_SIZE`, `FTH_INCLUDE_FLOAT`, `FTH_CASE_SENSITIVE`, etc.

## Testing

Tests live in `test/core-tests.fth` (142+ cases). The `test/test.fth` framework provides assertion words. Every new word must have corresponding tests; all tests must pass before a PR is submitted. Update `README.md` when adding new built-in words.
