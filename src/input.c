#include <readline/readline.h>
#include <stdlib.h>
#include <string.h>

#include "msh.h"

static int ends_with(const char *s, const char *suf) {
  size_t n = strlen(s), m = strlen(suf);
  return n >= m && strcmp(s + n - m, suf) == 0;
}

char *read_input(void) {
  char *line = readline(last_status ? "\001\033[31m\002msh> \001\033[0m\002"
                                    : "msh> ");
  if (line == NULL) return NULL;

  for (;;) {
    size_t n = strlen(line);
    while (n > 0 && (line[n - 1] == ' ' || line[n - 1] == '\t')) {
      line[--n] = '\0';
    }

    if (n > 0 && line[n - 1] == '\\') {
      line[--n] = '\0';
    } else if (!ends_with(line, "&&") && !ends_with(line, "||")) {
      break;
    }

    char *more = readline("> ");
    if (more == NULL) break;
    line = xrealloc(line, n + strlen(more) + 1);
    strcat(line, more);
    free(more);
  }
  return line;
}
