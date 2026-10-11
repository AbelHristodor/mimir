#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtins/builtins.h"
#include "msh.h"

static int is_op(const char *t) {
  return strcmp(t, ";") == 0 || strcmp(t, "&&") == 0 || strcmp(t, "||") == 0;
}

static int wait_child(pid_t pid) {
  int st;
  waitpid(pid, &st, WUNTRACED);
  return WIFEXITED(st) ? WEXITSTATUS(st) : 128 + WTERMSIG(st);
}

static int run_simple(char **argv) {
  static const struct {
    const char *name;
    int (*fn)(int, char **);
  } builtins[] = {
      {"cat", builtin_cat},
      {"echo", builtin_echo},
      {"ls", builtin_ls},
  };
  for (size_t i = 0; i < sizeof builtins / sizeof *builtins; i++) {
    if (strcmp(argv[0], builtins[i].name) == 0) {
      int argc = 0;
      while (argv[argc])
        argc++;
      return builtins[i].fn(argc, argv);
    }
  }
  if (strcmp(argv[0], "cd") == 0) {
    if (chdir(argv[1]) < 0) {
      perror(argv[1]);
      return 1;
    }
    return 0;
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  }
  if (pid == 0) {
    execvp(argv[0], argv);
    perror(argv[0]);
    exit(127);
  }
  return wait_child(pid);
}

// command := ['!'] ( '(' list ')' | word* )
// Executes a command
static int run_cmd(char ***tp, int enabled) {
  int negate = 0, status = 0;

  if (**tp != NULL && strcmp(**tp, "!") == 0) {
    negate = 1;
    (*tp)++;
  }

  if (**tp != NULL && strcmp(**tp, "(") == 0) {
    (*tp)++;
    if (enabled) {
      pid_t pid = fork();
      if (pid < 0) {
        perror("fork");
        status = 1;
      } else if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        exit(run_list(tp, 1));
      } else {
        status = wait_child(pid);
      }
    }
    run_list(tp, 0); // parent: step over the subshell's tokens
    if (**tp != NULL && strcmp(**tp, ")") == 0) {
      (*tp)++;
    } else if (enabled) {
      fprintf(stderr, "msh: expected ')'\n");
      status = 2;
    }
  } else {
    char **argv = *tp;
    while (**tp != NULL && !is_op(**tp) && strcmp(**tp, "(") != 0 &&
           strcmp(**tp, ")") != 0) {
      (*tp)++;
    }
    char *saved = **tp;
    **tp = NULL; // terminate argv in place, then put the operator back
    if (enabled && argv[0] != NULL)
      status = run_simple(argv);
    **tp = saved;
  }

  return negate ? !status : status;
}

// list := command ( (';' | '&&' | '||') command )*
int run_list(char ***tp, int enabled) {
  int status = 0, run = enabled;

  for (;;) {
    int s = run_cmd(tp, run);
    if (run)
      status = s;

    if (**tp == NULL || strcmp(**tp, ")") == 0)
      return status;

    char *op = *(*tp)++;
    if (!is_op(op)) {
      fprintf(stderr, "msh: syntax error near '%s'\n", op);
      while (**tp != NULL)
        (*tp)++;
      return 2;
    }
    if (strcmp(op, ";") == 0) {
      run = enabled;
    } else if (strcmp(op, "&&") == 0) {
      run = enabled && status == 0;
    } else {
      run = enabled && status != 0;
    }
  }
}
