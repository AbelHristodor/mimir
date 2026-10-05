#include <stdlib.h>
#include <string.h>

#include "msh.h"

char **tokenize(const char *s) {
  size_t cap = 16, n = 0;
  char **toks = xrealloc(NULL, cap * sizeof *toks);

  while (*s != '\0') {
    if (*s == ' ' || *s == '\t') {
      s++;
      continue;
    }
    size_t len;
    if (strchr("();", *s) != NULL) {
      len = 1;
    } else if ((s[0] == '&' && s[1] == '&') || (s[0] == '|' && s[1] == '|')) {
      len = 2;
    } else {
      len = strcspn(s, " \t();&|");
      if (len == 0) len = 1;  // lone & or |, keep as a word for now
    }
    if (n + 1 >= cap) toks = xrealloc(toks, (cap *= 2) * sizeof *toks);
    toks[n++] = strndup(s, len);
    s += len;
  }
  toks[n] = NULL;
  return toks;
}

void free_tokens(char **toks) {
  for (char **t = toks; *t != NULL; t++) free(*t);
  free(toks);
}
