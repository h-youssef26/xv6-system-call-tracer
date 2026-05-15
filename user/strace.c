#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define TRACE_ALL 0x7fffffff

struct {
  char *name;
  int num;
} syscall_map[] = {
  {"fork", 1},
  {"exit", 2},
  {"wait", 3},
  {"pipe", 4},
  {"read", 5},
  {"kill", 6},
  {"exec", 7},
  {"fstat", 8},
  {"chdir", 9},
  {"dup", 10},
  {"getpid", 11},
  {"sbrk", 12},
  {"pause", 13},
  {"uptime", 14},
  {"open", 15},
  {"write", 16},
  {"mknod", 17},
  {"unlink", 18},
  {"link", 19},
  {"mkdir", 20},
  {"close", 21},
  {"trace", 22},
};

int
syscall_name_to_num(char *name)
{
  for(int i = 0; i < sizeof(syscall_map) / sizeof(syscall_map[0]); i++){
    if(strcmp(syscall_map[i].name, name) == 0){
      return syscall_map[i].num;
    }
  }
  return -1;
}

int
main(int argc, char *argv[])
{
  int mask;
  int start;

  if(argc < 2){
    fprintf(2, "Usage: strace [-a | -e trace=syscall | mask] command [args...]\n");
    exit(1);
  }

  if(strcmp(argv[1], "-a") == 0){
    mask = TRACE_ALL;
    start = 2;
  } else if(strcmp(argv[1], "-e") == 0){
    if(argc < 4){
      fprintf(2, "strace: -e option must be of form: -e trace=syscall\n");
      exit(1);
    }

    char *arg = argv[2];
    if(strncmp(arg, "trace=", 6) != 0){
      fprintf(2, "strace: -e option must be of form: -e trace=syscall\n");
      exit(1);
    }

    char *syscall_name = arg + 6;
    if(strlen(syscall_name) == 0){
      fprintf(2, "strace: missing syscall name\n");
      exit(1);
    }

    int num = syscall_name_to_num(syscall_name);
    if(num == -1){
      fprintf(2, "strace: unknown syscall: %s\n", syscall_name);
      exit(1);
    }

    mask = (1 << num);
    start = 3;
  } else if(argv[1][0] >= '0' && argv[1][0] <= '9'){
    mask = atoi(argv[1]);
    start = 2;
  } else {
    mask = TRACE_ALL;
    start = 1;
  }

  if(start >= argc){
    fprintf(2, "strace: missing command\n");
    exit(1);
  }

  trace(mask);

  argv[start + argc - start] = 0;

  if(exec(argv[start], argv + start) < 0){
    fprintf(2, "strace: exec %s failed\n", argv[start]);
  }

  exit(0);

}
