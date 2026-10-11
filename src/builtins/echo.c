#include <stdio.h>
#include <stdlib.h>

int builtin_echo(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (i > 1)
      putchar(' ');
    fputs(argv[i], stdout); // this is buffered
  }
  putchar('\n');
  fflush(stdout); // gotta flush the buffer
  return EXIT_SUCCESS;
}
