# msh

A small Unix shell in C, built one stage at a time. Uses `readline` for input
and `fork`/`exec`/`wait` for everything else.

## Build

```sh
make release          # build/release/main
make debug            # build/debug/main, -O0 -g3
make asan             # AddressSanitizer build plus a catch test
make ubsan            # UndefinedBehaviorSanitizer build plus a catch test
make tsan             # ThreadSanitizer build plus a catch test
make all              # every configuration
make clean
```

Requires a C compiler and the readline development headers.

## Run

```sh
./build/release/main
```

Exit with Ctrl-D. The prompt turns red when the last command returned a
non zero status.

## What works

- Commands of any length, found via `PATH`
- Builtin `cd`
- Exit status from `wait`, and `!` to negate it
- Lists: `;`, `&&`, `||`
- Subshells: `(cd /tmp && pwd); pwd`
- Line continuation with a trailing `\`, `&&`, or `||`
- Ctrl-C returns to the prompt

```
msh> false && echo foo || echo bar
bar
msh> (cd /tmp && pwd); pwd
/tmp
/home/me
msh> ! true
msh> echo one \
> two
one two
```

## Not yet

Quoting, variables, redirection, pipes, background jobs.

## Layout

```
src/main.c    SIGINT handling and the read, tokenize, run loop
src/input.c   read_input: prompt and line continuation
src/lexer.c   tokenize: words and the operators ( ) ; && ||
src/exec.c    run_list, run_cmd, run_simple: lists, subshells, !
src/util.c    xrealloc
src/msh.h     shared declarations
tests/        run.sh and the deliberately buggy sanitizer programs
```
