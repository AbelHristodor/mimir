# msh

A small Unix shell in C. For learning purposes.

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
- Pipes: `echo hi | tr a-z A-Z | cat`
- Redirection: `<`, `>`, `>>`, with an fd prefix like `2> err.log`
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
msh> ! ls /nope 2> /dev/null | wc -l
       0
```

A command with a pipe or a redirect runs in a child, builtins included, so
`cd /tmp > log` does not change the shell's directory.

## Not yet

Quoting, variables, `2>&1`, heredocs, background jobs.

## Layout

```
src/main.c    SIGINT handling and the read, tokenize, run loop
src/input.c   read_input: prompt and line continuation
src/lexer.c   tokenize: words and the operators ( ) ; && || | < > >>
src/exec.c    run_list, run_pipeline, parse_cmd: lists, pipes, redirects, subshells, !
src/util.c    xrealloc
src/msh.h     shared declarations
tests/        run.sh, shell.sh behaviour tests, the deliberately buggy sanitizer programs
```
