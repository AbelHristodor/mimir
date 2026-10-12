#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int builtin_pwd(int argc) {

  // Doesn't take any arguments.
  if (argc != 1)
    return EXIT_FAILURE;

  char *path = getcwd(NULL, 0);
  printf("%s\n", path);

  return EXIT_SUCCESS;
}
