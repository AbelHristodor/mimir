#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 1024

// implementation of "cat" command
// buffered
static int cat_fd(int read_fd) {

  char buffer[BUF_SIZE];

  while (1) {

    ssize_t bytes_read = read(read_fd, buffer, BUF_SIZE);
    if (bytes_read < 0) {
      perror("could not read file");
      return EXIT_FAILURE;
    }

    if (bytes_read == 0) {
      // We reached EOF, nothing to print out
      return EXIT_SUCCESS;
    }

    if (write(STDOUT_FILENO, buffer, bytes_read) != bytes_read) {
      perror("error writing");
      return EXIT_FAILURE;
    }
  }
}

// cat [FILE...]: concatenate files (or stdin, or "-") to stdout.
int builtin_cat(int argc, char *argv[]) {

  if (argc == 1)
    return cat_fd(STDIN_FILENO);

  int status = EXIT_SUCCESS;

  // We loop over each argument
  for (int i = 1; i < argc; i++) {

    // - means stding, otherwise open the file
    int fd = strcmp(argv[i], "-") == 0 ? STDIN_FILENO : open(argv[i], O_RDONLY);
    if (fd == -1) {
      perror(argv[i]);
      return EXIT_FAILURE;
    }

    if (cat_fd(fd) != EXIT_SUCCESS)
      status = EXIT_FAILURE;

    if (fd != STDIN_FILENO)
      close(fd);
  }

  return status;
}
