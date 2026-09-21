#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

// Lab2 task 3: sys_yield() walks the global process table.
extern struct proc proc[NPROC];

uint64 sys_exit(void) {
  int n;
  if (argint(0, &n) < 0) return -1;
  exit(n);
  return 0;  // not reached
}

uint64 sys_getpid(void) { return myproc()->pid; }

uint64 sys_fork(void) { return fork(); }

uint64 sys_wait(void) {
  uint64 p;
  int flags;
  if (argaddr(0, &p) < 0) return -1;
  // Lab2 task 2: second argument selects blocking (0) or non-blocking (!= 0).
  if (argint(1, &flags) < 0) return -1;
  return wait(p, flags);
}

uint64 sys_sbrk(void) {
  int addr;
  int n;

  if (argint(0, &n) < 0) return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0) return -1;
  return addr;
}

uint64 sys_sleep(void) {
  int n;
  uint ticks0;

  if (argint(0, &n) < 0) return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (myproc()->killed) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64 sys_kill(void) {
  int pid;

  if (argint(0, &pid) < 0) return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64 sys_uptime(void) {
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_rename(void) {
  char name[16];
  int len = argstr(0, name, MAXPATH);
  if (len < 0) {
    return -1;
  }
  struct proc *p = myproc();
  memmove(p->name, name, len);
  p->name[len] = '\0';
  return 0;
}

// Lab2 task 3: the yield() system call.
// Reports where the kernel thread context gets saved, which process is running
// right now, and which process the scheduler is likely to run next; then gives
// up the CPU through the kernel's own yield().
// NOTE: run yieldtest with CPUS=1, otherwise the output of the CPUs interleaves.
uint64 sys_yield(void) {
  struct proc *p = myproc();

  printf("Save the context of the process to the memory region from address %p to %p\n",
         &p->context, &p->context + 1);
  printf("Current running process pid is %d and user pc is %p\n", p->pid, p->trapframe->epc);

  // Walk the process table starting right after p, the same way scheduler()
  // does, and report the first RUNNABLE process found.
  for (int i = 1; i <= NPROC; i++) {
    struct proc *np = &proc[(p - proc + i) % NPROC];
    if (np == p) continue;
    acquire(&np->lock);
    if (np->state == RUNNABLE) {
      printf("Next runnable process pid is %d and user pc is %p\n", np->pid, np->trapframe->epc);
      release(&np->lock);
      break;
    }
    release(&np->lock);
  }

  yield();
  return 0;
}
