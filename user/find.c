#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

// Lab1 find:
//   find <path> <name>
// Walks the directory tree under <path>, printing the relative path of every
// entry whose own name equals <name>.  Directories are matched too, and a
// matching directory is printed without being searched inside it (that would
// only produce more paths we were not asked for).

char *fmtname(char *path) {
  char *p;

  // Last component of the path.
  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  return p + 1;
}

void find(char *path, char *name) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type) {
    case T_FILE:
      // A plain file whose own name matches.
      if (strcmp(fmtname(path), name) == 0) printf("%s\n", path);
      break;

    case T_DIR:
      if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
        fprintf(2, "find: path too long: %s\n", path);
        break;
      }

      // The starting directory itself may be the entry being searched for.
      if (strcmp(fmtname(path), name) == 0) printf("%s\n", path);

      strcpy(buf, path);
      p = buf + strlen(buf);
      *p++ = '/';
      while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0) continue;  // empty slot in the directory
        // Never recurse into "." or "..": that would loop forever.
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;

        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;  // dirent.name is not NUL-terminated

        if (stat(buf, &st) < 0) {
          fprintf(2, "find: cannot stat %s\n", buf);
          continue;
        }

        if (st.type == T_DIR) {
          // A matching directory is reported here, so once it matches we do
          // not descend into it.
          if (strcmp(de.name, name) == 0) {
            printf("%s\n", buf);
          } else {
            find(buf, name);
          }
        } else if (strcmp(de.name, name) == 0) {
          printf("%s\n", buf);
        }
      }
      break;
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(2, "usage: find <path> <name>\n");
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}
