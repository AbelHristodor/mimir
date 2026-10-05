#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "msh.h"

int last_status = 0;

static sigjmp_buf env;
static volatile sig_atomic_t jump_active = 0;

static void sigint_handler(int signo) {
  (void)signo;
  if (!jump_active)
    return;
  siglongjmp(env, 42);
}

int main(void) {
  // Signal handling
  struct sigaction s;
  s.sa_handler = sigint_handler;
  sigemptyset(&s.sa_mask);
  s.sa_flags = SA_RESTART;
  sigaction(SIGINT, &s, NULL);

  while (1) {

    // Handle re-start of the loop when a signal is handled instead of exiting.
    if (sigsetjmp(env, 1) == 42) {
      printf("\n");
      continue;
    }
    jump_active = 1;

    // Reading input
    char *input = read_input();
    if (input == NULL) { // Ctrl-D
      printf("\n");
      break;
    }

    // Tokenize the input, i.e. parse it.
    char **toks = tokenize(input);
    char **tp = toks;

    last_status = run_list(&tp, 1);
    if (*tp != NULL) {
      fprintf(stderr, "msh: syntax error near ')'\n");
      last_status = 2;
    }

    free_tokens(toks);
    free(input);
  }

  return last_status;
}
