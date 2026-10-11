#ifndef BUILTINS_H
#define BUILTINS_H

// Each builtin takes argv like a program would (argv[0] is the command name)
// and returns an exit status.
int builtin_cat(int argc, char *argv[]);
int builtin_echo(int argc, char *argv[]);
int builtin_ls(int argc, char *argv[]);

#endif
