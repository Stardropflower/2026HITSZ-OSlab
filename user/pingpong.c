#include "kernel/types.h"
#include "user/user.h"

// Lab1 pingpong:
//   parent --"ping"--> child --"pong"--> parent
// A pipe is one-way, so two pipes are needed.
// Output format required by grade-lab-util:
//   <pid>: received ping from pid <pid>
//   <pid>: received pong from pid <pid>

int main(int argc, char *argv[]) {
  int ptoc[2];  // parent -> child
  int ctop[2];  // child  -> parent
  char buf[8];   // 4 bytes of message + room for the NUL printf needs
  int pid;
  int mypid = getpid();  // xv6 has no getppid(), so remember it before fork

  if (pipe(ptoc) < 0 || pipe(ctop) < 0) {
    fprintf(2, "pingpong: pipe failed\n");
    exit(1);
  }

  pid = fork();
  if (pid < 0) {
    fprintf(2, "pingpong: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // Child: read the ping from the parent, then send a pong back.
    close(ptoc[1]);
    close(ctop[0]);

    if (read(ptoc[0], buf, 4) != 4) {
      fprintf(2, "pingpong: child read failed\n");
      exit(1);
    }
    buf[4] = '\0';  // make it a real C string for printf
    printf("%d: received %s from pid %d\n", getpid(), buf, mypid);
    if (write(ctop[1], "pong", 4) != 4) {
      fprintf(2, "pingpong: child write failed\n");
      exit(1);
    }

    close(ptoc[0]);
    close(ctop[1]);
    exit(0);
  }

  // Parent: send a ping to the child, then wait for the pong.
  close(ptoc[0]);
  close(ctop[1]);

  if (write(ptoc[1], "ping", 4) != 4) {
    fprintf(2, "pingpong: parent write failed\n");
    exit(1);
  }
  if (read(ctop[0], buf, 4) != 4) {
    fprintf(2, "pingpong: parent read failed\n");
    exit(1);
  }
  buf[4] = '\0';
  printf("%d: received %s from pid %d\n", getpid(), buf, pid);

  close(ptoc[1]);
  close(ctop[0]);
  wait(0);
  exit(0);
}
