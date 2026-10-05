#ifndef MSH_H
#define MSH_H

#include <stddef.h>

// Exit status of the last list run at top level. Drives the prompt colour.
extern int last_status;

// util.c
void *xrealloc(void *p, size_t n);

// input.c
// Read one logical line, following '\' and trailing && / || continuations.
// Returns NULL on EOF. Caller frees.
char *read_input(void);

// lexer.c
// Split a line into words and the operators ( ) ; && ||.
// Returns a NULL-terminated array of malloc'd strings.
char **tokenize(const char *s);
void free_tokens(char **toks);

// exec.c
// Run a list starting at *tp. With enabled == 0 only advances past it.
// Returns the exit status of the last command run.
int run_list(char ***tp, int enabled);

#endif
