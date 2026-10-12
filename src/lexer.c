#include <ctype.h>
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
    // A digit glued to a redirect is part of it: 2> is one token, 2 > is two.
    const char *r = isdigit((unsigned char)s[0]) ? s + 1 : s;
    if (r[0] == '>' && r[1] == '>') {
      len = (size_t)(r - s) + 2;
    } else if (r[0] == '<' || r[0] == '>') {
      len = (size_t)(r - s) + 1;
    } else if (strchr("();|", *s) != NULL) {
      len = 1;
    } else if ((s[0] == '&' && s[1] == '&') || (s[0] == '|' && s[1] == '|')) {
      len = 2;
    } else {
      len = strcspn(s, " \t();&|<>");
      if (len == 0) len = 1;  // lone &, keep as a word for now
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
