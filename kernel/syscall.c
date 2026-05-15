#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

// Fetch the uint64 at addr from the current process.
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if(addr >= p->sz || addr+sizeof(uint64) > p->sz)
    return -1;
  if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  if(copyinstr(p->pagetable, buf, addr, max) < 0)
    return -1;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
void
argint(int n, int *ip)
{
  *ip = argraw(n);
}

// Retrieve an argument as a pointer.
void
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  argaddr(n, &addr);
  return fetchstr(addr, buf, max);
}

// Prototypes for the functions that handle system calls.
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_kill(void);
extern uint64 sys_exec(void);
extern uint64 sys_fstat(void);
extern uint64 sys_chdir(void);
extern uint64 sys_dup(void);
extern uint64 sys_getpid(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_pause(void);
extern uint64 sys_uptime(void);
extern uint64 sys_open(void);
extern uint64 sys_write(void);
extern uint64 sys_mknod(void);
extern uint64 sys_unlink(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_close(void);
extern uint64 sys_trace(void);
extern uint64 sys_getcounts(void);

// An array mapping syscall numbers from syscall.h
// to the function that handles the system call.
static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_pause]   sys_pause,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_trace]   sys_trace,
[SYS_getcounts] sys_getcounts,
};

// Helper to print a single character with proper escaping
static void
print_char(unsigned char c)
{
  if(c >= 32 && c < 127) {
    if(c == '"') printf("\\\"");
    else if(c == '\\') printf("\\\\");
    else printf("%c", c);
  } else if(c == '\n') {
    printf("\\n");
  } else if(c == '\t') {
    printf("\\t");
  } else if(c == '\r') {
    printf("\\r");
  } else if(c == '\0') {
    printf("\\0");
  } else {
    printf("\\x%02x", c);
  }
}

// Print syscall arguments based on syscall type
static void
print_syscall_args(int num, struct trapframe *tf)
{
  char buf[256];
  char path[256];
  int i;
  
  switch(num) {
    case SYS_fork:
      printf("fork()");
      break;
      
    case SYS_exit:
      printf("exit(%ld)", tf->a0);
      break;
      
    case SYS_wait:
      printf("wait(%ld)", tf->a1);
      break;
      
    case SYS_pipe:
      printf("pipe(%ld)", tf->a0);
      break;
      
    case SYS_read:
      printf("read(%ld, %ld, %ld)", tf->a0, tf->a1, tf->a2);
      break;
      
    case SYS_kill:
      printf("kill(%ld)", tf->a0);
      break;
      
    case SYS_exec:
      if(copyinstr(myproc()->pagetable, path, tf->a0, 256) >= 0) {
        printf("exec(\"%s\", %ld)", path, tf->a1);
      } else {
        printf("exec(?, %ld)", tf->a1);
      }
      break;
      
    case SYS_fstat:
      printf("fstat(%ld, %ld)", tf->a0, tf->a1);
      break;
      
    case SYS_chdir:
      if(copyinstr(myproc()->pagetable, path, tf->a0, 256) >= 0) {
        printf("chdir(\"%s\")", path);
      } else {
        printf("chdir(?)");
      }
      break;
      
    case SYS_dup:
      printf("dup(%ld)", tf->a0);
      break;
      
    case SYS_getpid:
      printf("getpid()");
      break;
      
    case SYS_sbrk:
      printf("sbrk(%ld)", tf->a0);
      break;

    case SYS_pause:
      printf("pause()");
      break;
      
    case SYS_uptime:
      printf("uptime()");
      break;
      
    case SYS_open:
      if(copyinstr(myproc()->pagetable, path, tf->a0, 256) >= 0) {
        printf("open(\"%s\", %ld)", path, tf->a1);
      } else {
        printf("open(?, %ld)", tf->a1);
      }
      break;
      
    case SYS_write:
      printf("write(%ld, \"", tf->a0);
      
      // Use copyin() for arbitrary binary data
      int print_len = tf->a2 < 40 ? tf->a2 : 40;
      
      if(copyin(myproc()->pagetable, buf, tf->a1, print_len) >= 0) {
        for(i = 0; i < print_len; i++) {
          print_char((unsigned char)buf[i]);
        }
        if(print_len < tf->a2) {
          printf("...");
        }
      } else {
        printf("?");
      }
      printf("\", %ld)", tf->a2);
      break;
      
    case SYS_mknod:
      if(copyinstr(myproc()->pagetable, path, tf->a0, 256) >= 0) {
        printf("mknod(\"%s\", %ld, %ld)", path, tf->a1, tf->a2);
      } else {
        printf("mknod(?, %ld, %ld)", tf->a1, tf->a2);
      }
      break;
      
    case SYS_unlink:
      if(copyinstr(myproc()->pagetable, path, tf->a0, 256) >= 0) {
        printf("unlink(\"%s\")", path);
      } else {
        printf("unlink(?)");
      }
      break;
      
    case SYS_link:
      if(copyinstr(myproc()->pagetable, path, tf->a0, 256) >= 0) {
        printf("link(\"%s\", ", path);
      } else {
        printf("link(?, ");
      }
      
      if(copyinstr(myproc()->pagetable, buf, tf->a1, 256) >= 0) {
        printf("\"%s\")", buf);
      } else {
        printf("?)");
      }
      break;
      
    case SYS_mkdir:
      if(copyinstr(myproc()->pagetable, path, tf->a0, 256) >= 0) {
        printf("mkdir(\"%s\")", path);
      } else {
        printf("mkdir(?)");
      }
      break;
      
    case SYS_close:
      printf("close(%ld)", tf->a0);
      break;
      
    case SYS_trace:
      printf("trace(%ld)", tf->a0);
      break;
      
    default:
      printf("syscall_%d", num);
  }
}

void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    // Check if this syscall should be traced
    if(p->trace_mask & (1 << num)) {
      print_syscall_args(num, p->trapframe);
    }
    
    // Call the actual syscall
    p->trapframe->a0 = syscalls[num]();
    
    if(num > 0 && num < 23)
    p->syscall_counts[num]++;


    // Print return value if tracing
    if(p->trace_mask & (1 << num)) {
      if(num == SYS_exit) {
        printf(" = ?\n");
      } else if(num == SYS_exec) {
        printf(" = -1 (ENOEXEC)\n");
      } else {
        printf(" = %ld\n", p->trapframe->a0);
      }
    }
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
