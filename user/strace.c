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

  // -c flag: count mode
 if(strcmp(argv[1], "-c") == 0){
    if(argc < 3){
        fprintf(2, "Usage: strace -c <command>\n");
        exit(1);
    }

    char *sysnames[] = {
        "",
        "fork", "exit", "wait", "pipe", "read",
        "kill", "exec", "fstat", "chdir", "dup",
        "getpid", "sbrk", "pause", "uptime", "open",
        "write", "mknod", "unlink", "link", "mkdir",
        "close", "trace", "getcounts"
    };

    int before[24];
    int after[24];

    // get counts before in THIS process
    getcounts(before);

    // run the command in this same process after fork
    int pid = fork();
    if(pid == 0){
        exec(argv[2], &argv[2]);
        fprintf(2, "strace: exec %s failed\n", argv[2]);
        exit(1);
    }
    wait(0);

    // get counts after
    getcounts(after);

    printf("syscall              count\n");
    printf("----------------------------\n");
    for(int i = 1; i < 24; i++){
        int diff = after[i] - before[i];
        if(diff > 0)
            printf("%s: %d\n", sysnames[i], diff);
    }
    exit(0);
}

  if(strcmp(argv[1], "-a") == 0){
    mask = TRACE_ALL;
    start = 2;
  } else if(strcmp(argv[1], "-e") == 0){
    if(argc < 4){
      fprintf(2, "strace: -e option must be of form: -e trace=syscall[,syscall...] command [args...]\n");
      exit(1);
    }

    char *arg = argv[2];
    if(strncmp(arg, "trace=", 6) != 0){
      fprintf(2, "strace: -e option must be of form: -e trace=syscall[,syscall...]\n");
      exit(1);
    }

    char *syscall_list = arg + 6;
    if(strlen(syscall_list) == 0){
      fprintf(2, "strace: missing syscall name\n");
      exit(1);
    }

    mask = 0;
    char *start_name = syscall_list;
    for(int i = 0; ; i++){
      char c = syscall_list[i];
      if(c == ',' || c == '\0'){
        int len = syscall_list + i - start_name;
        char name_buf[128];
        if(len >= (int)sizeof(name_buf)){
          fprintf(2, "strace: syscall name too long\n");
          exit(1);
        }
        strncpy(name_buf, start_name, len);
        name_buf[len] = '\0';

        int num = syscall_name_to_num(name_buf);
        if(num == -1){
          fprintf(2, "strace: unknown syscall: %s\n", name_buf);
          exit(1);
        }

        mask |= (1 << num);

        if(c == '\0') break;
        start_name = syscall_list + i + 1;
      }
    }

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
