#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtins/builtins.h"
#include "msh.h"

#define MAX_REDIR 8

struct redir {
  int fd;
  int flags;
  const char *path;
};

// One pipeline stage. Either a subshell (sub points at the tokens after '(')
// or a simple command (argv, malloc'd, NULL-terminated).
struct cmd {
  char **argv;
  char **sub;
  struct redir redir[MAX_REDIR];
  int nredir;
};

static int is_list_op(const char *t) {
  return strcmp(t, ";") == 0 || strcmp(t, "&&") == 0 || strcmp(t, "||") == 0;
}

static int is_op(const char *t) {
  return is_list_op(t) || strcmp(t, "|") == 0 || strcmp(t, "(") == 0 ||
         strcmp(t, ")") == 0;
}

// Fills r and returns 1 if t is a redirect operator such as < > >> 2> 2>>.
static int parse_redir_op(const char *t, struct redir *r) {
  const char *op = t;
  int fd = -1;
  if (t[0] >= '0' && t[0] <= '9') {
    fd = t[0] - '0';
    op = t + 1;
  }
  if (strcmp(op, "<") == 0) {
    r->fd = fd < 0 ? 0 : fd;
    r->flags = O_RDONLY;
  } else if (strcmp(op, ">") == 0) {
    r->fd = fd < 0 ? 1 : fd;
    r->flags = O_WRONLY | O_CREAT | O_TRUNC;
  } else if (strcmp(op, ">>") == 0) {
    r->fd = fd < 0 ? 1 : fd;
    r->flags = O_WRONLY | O_CREAT | O_APPEND;
  } else {
    return 0;
  }
  return 1;
}

static int syntax_error(char ***tp, const char *near) {
  fprintf(stderr, "msh: syntax error near '%s'\n", near ? near : "end of line");
  while (**tp != NULL)
    (*tp)++;
  return -1;
}

// command := '(' list ')' redir* | ( word | redir )*
// Parses one stage into c, advancing *tp. Returns -1 on syntax error.
static int parse_cmd(char ***tp, struct cmd *c) {
  size_t cap = 8, n = 0;
  c->argv = xrealloc(NULL, cap * sizeof *c->argv);
  c->argv[0] = NULL;
  c->sub = NULL;
  c->nredir = 0;

  if (**tp != NULL && strcmp(**tp, "(") == 0) {
    c->sub = ++*tp;
    run_list(tp, 0);
    if (**tp == NULL || strcmp(**tp, ")") != 0)
      return syntax_error(tp, **tp);
    (*tp)++;
  }

  while (**tp != NULL && !is_op(**tp)) {
    struct redir r;
    if (parse_redir_op(**tp, &r)) {
      char *path = *++*tp;
      if (path == NULL || is_op(path) || parse_redir_op(path, &r))
        return syntax_error(tp, path);
      if (c->nredir == MAX_REDIR)
        return syntax_error(tp, path);
      r.path = path;
      c->redir[c->nredir++] = r;
    } else if (c->sub != NULL) {
      return syntax_error(tp, **tp);
    } else {
      if (n + 1 >= cap)
        c->argv = xrealloc(c->argv, (cap *= 2) * sizeof *c->argv);
      c->argv[n++] = **tp;
      c->argv[n] = NULL;
    }
    (*tp)++;
  }
  return 0;
}

static int apply_redirs(const struct cmd *c) {
  for (int i = 0; i < c->nredir; i++) {
    const struct redir *r = &c->redir[i];
    int fd = open(r->path, r->flags, 0666);
    if (fd < 0) {
      perror(r->path);
      return -1;
    }
    if (fd != r->fd) {
      dup2(fd, r->fd);
      close(fd);
    }
  }
  return 0;
}

static int wait_child(pid_t pid) {
  int st;
  waitpid(pid, &st, WUNTRACED);
  return WIFEXITED(st) ? WEXITSTATUS(st) : 128 + WTERMSIG(st);
}

// Runs argv in this process if it is a builtin. Returns -1 otherwise.
static int run_builtin(char **argv) {
  static const struct {
    const char *name;
    int (*fn)(int, char **);
  } builtins[] = {
      {"cat", builtin_cat},
      {"echo", builtin_echo},
      {"ls", builtin_ls},
      {"pwd", builtin_pwd},
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
    if (argv[1] == NULL || chdir(argv[1]) < 0) {
      perror(argv[1] ? argv[1] : "cd");
      return 1;
    }
    return 0;
  }
  return -1;
}

// Child side of a stage
static void exec_cmd(const struct cmd *c) {
  signal(SIGINT, SIG_DFL);
  if (apply_redirs(c) < 0)
    exit(1);
  if (c->sub != NULL) {
    char **tp = c->sub;
    exit(run_list(&tp, 1));
  }
  if (c->argv[0] == NULL)
    exit(0);
  int st = run_builtin(c->argv);
  if (st >= 0)
    exit(st);
  execvp(c->argv[0], c->argv);
  perror(c->argv[0]);
  exit(127);
}

// pipeline := ['!'] command ( '|' command )*
static int run_pipeline(char ***tp, int enabled) {
  int negate = 0, status = 0, prev = -1, nforked = 0;
  pid_t last = -1;

  if (**tp != NULL && strcmp(**tp, "!") == 0) {
    negate = 1;
    (*tp)++;
  }

  for (;;) {
    struct cmd c;
    int bad = parse_cmd(tp, &c);
    int more = !bad && **tp != NULL && strcmp(**tp, "|") == 0;
    int plain = !more && nforked == 0 && c.sub == NULL && c.nredir == 0;

    if (bad) {
      free(c.argv);
      status = 2;
      break;
    }
    if (!enabled) {
      free(c.argv);
    } else if (plain &&
               (c.argv[0] == NULL || (status = run_builtin(c.argv)) >= 0)) {
      free(c.argv);
    } else {
      int fds[2] = {-1, -1};
      if (more && pipe(fds) < 0) {
        perror("pipe");
        free(c.argv);
        status = 1;
        break;
      }
      pid_t pid = fork();
      if (pid < 0) {
        perror("fork");
        free(c.argv);
        status = 1;
        break;
      }
      if (pid == 0) {
        if (prev != -1) {
          dup2(prev, 0);
          close(prev);
        }
        if (more) {
          dup2(fds[1], 1);
          close(fds[0]);
          close(fds[1]);
        }
        exec_cmd(&c);
      }
      free(c.argv);
      if (prev != -1)
        close(prev);
      if (more) {
        close(fds[1]);
        prev = fds[0];
      }
      last = pid;
      nforked++;
    }

    if (!more)
      break;
    (*tp)++;
  }

  if (prev != -1)
    close(prev);
  if (nforked > 0) {
    status = wait_child(last);
    while (--nforked > 0)
      wait(NULL);
  }
  return negate ? !status : status;
}

// list := pipeline ( (';' | '&&' | '||') pipeline )*
int run_list(char ***tp, int enabled) {
  int status = 0, run = enabled;

  for (;;) {
    int s = run_pipeline(tp, run);
    if (run)
      status = s;

    if (**tp == NULL || strcmp(**tp, ")") == 0)
      return status;

    char *op = *(*tp)++;
    if (!is_list_op(op)) {
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
