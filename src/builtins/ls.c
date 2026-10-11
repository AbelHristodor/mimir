#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "builtins/builtins.h"

// "-rwxr-xr-x" style permission string.
static void mode_str(mode_t m, char out[11]) {
  out[0] = S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' : '-';
  const char *rwx = "rwxrwxrwx";
  for (int i = 0; i < 9; i++)
    out[i + 1] = (m & (1 << (8 - i))) ? rwx[i] : '-';
  out[10] = '\0';
}

int builtin_ls(int argc, char *argv[]) {

  const char *path = argc > 1 ? argv[1] : ".";

  DIR *dirp = opendir(path);
  if (dirp == NULL) {
    perror(path);
    return EXIT_FAILURE;
  }

  struct dirent *dp;
  struct stat st;
  while ((dp = readdir(dirp)) != NULL) {
    if (dp->d_name[0] == '.')
      continue; // hide dotfiles like plain ls

    // AT_SYMLINK_NOFOLLOW so a symlink shows as 'l', like ls -l.
    if (fstatat(dirfd(dirp), dp->d_name, &st, AT_SYMLINK_NOFOLLOW) == -1) {
      perror(dp->d_name);
      return EXIT_FAILURE;
    }

    char mode[11];
    mode_str(st.st_mode, mode);

    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);

    char when[32];
    strftime(when, sizeof when, "%b %e %H:%M", localtime(&st.st_mtime));

    printf("%s %3u %-8s %-8s %8lld %s %s\n", mode, (unsigned)st.st_nlink,
           pw ? pw->pw_name : "?", gr ? gr->gr_name : "?",
           (long long)st.st_size, when, dp->d_name);
  }

  (void)closedir(dirp);
  fflush(stdout);

  return EXIT_SUCCESS;
}
