#include <stdio.h>
#include <stdlib.h>

#include "msh.h"

void *xrealloc(void *p, size_t n) {
  p = realloc(p, n);
  if (p == NULL) {
    perror("realloc");
    exit(1);
  }
  return p;
}
