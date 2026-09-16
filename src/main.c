#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MSH_RL_BUFSIZE 1024

// Defines whether the current session is interactive or not
static int interactive(void) {
  return isatty(STDIN_FILENO) && isatty(STDERR_FILENO);
}

char *read_line(void) {

  int bufsize = MSH_RL_BUFSIZE;
  // Create buffer for max size input in the shell
  char *buffer = malloc(sizeof(char) * bufsize);
  if (!buffer) {
    fprintf(stderr, "msh: allocation error while reading\n");
    exit(EXIT_FAILURE);
  }

  if (interactive()) {
    fputs("msh> ", stderr);
  }

  // Read one char at a time until end of line or end of input.
  size_t pos = 0;
  for (;;) {
    char c;
    ssize_t size_read = read(STDIN_FILENO, &c, 1);

    if (size_read == 0) { // No bytes to read, we're good to end
      break;
    }

    if (size_read < 0) {
      // A signal interrupted the read
      if (errno == EINTR) {
        continue;
      }
      perror("msh: read");
      free(buffer);
      exit(EXIT_FAILURE);
    }

    if (c == '\n') {
      break;
    }

    if (pos + 1 >= MSH_RL_BUFSIZE) {
      fprintf(stderr, "msh: line longer than %d bytes\n", MSH_RL_BUFSIZE - 1);
      fprintf(stderr, "\t reallocating...");

      bufsize += MSH_RL_BUFSIZE;
      buffer = realloc(buffer, 2 * MSH_RL_BUFSIZE);

      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    buffer[pos] = c;
    pos++;
  }

  buffer[pos] = '\0';
  return buffer;
}

int main() {

  char *lines = read_line();

  int c = 0;
  do {
    printf("%c", *lines);
    c++;
  } while (lines[c] != '\0');

  return EXIT_SUCCESS;
}
