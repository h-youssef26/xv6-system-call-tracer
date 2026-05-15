#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define TRACE_ALL 0x00ffffff

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
  int mask = -1; // -1 indicates no mask set yet via -a, -e
  int count_summary = 0;
  char *output_file = 0;
  int i = 1;

  if(argc < 2){
    fprintf(2, "Usage: strace [-o file] [-c] [-a | -e trace=syscall | mask] <command> [args...]\n");
    exit(1);
  }

  // Loop through options 
  while(i < argc && argv[i][0] == '-') {
    if(strcmp(argv[i], "-o") == 0){
      if(i + 1 >= argc){
        fprintf(2, "strace: -o requires a file name\n");
        exit(1);
      }
      output_file = argv[i+1];
      i += 2;
    } else if(strcmp(argv[i], "-c") == 0){
      count_summary = 1;
      i += 1;
    } else if(strcmp(argv[i], "-a") == 0){
      mask = TRACE_ALL;
      i += 1;
    } else if(strcmp(argv[i], "-e") == 0){
      if(i + 1 >= argc){
        fprintf(2, "strace: -e option must be of form: -e trace=syscall[,syscall]\n");
        exit(1);
      }

      char *arg = argv[i+1];
      if(strncmp(arg, "trace=", 6) != 0){
        fprintf(2, "strace: -e option must be of form: -e trace=syscall[,syscall]\n");
        exit(1);
      }

      char *syscall_list = arg + 6;
      if(strlen(syscall_list) == 0){
        fprintf(2, "strace: missing syscall name\n");
        exit(1);
      }

      mask = 0;
      char *start_name = syscall_list;
      for(int j = 0; ; j++){
        char c = syscall_list[j];
        if(c == ',' || c == '\0'){
          int len = syscall_list + j - start_name;
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
          start_name = syscall_list + j + 1;
        }
      }
      i += 2;
    } else {
      break;
    }
  }

  // Fallback check for raw numeric mask if -a or -e weren't passed
  if(mask == -1) {
    if(i < argc && argv[i][0] >= '0' && argv[i][0] <= '9') {
      mask = atoi(argv[i]);
      i++;
    } else {
      mask = TRACE_ALL; 
    }
  }

  if(i >= argc){
    fprintf(2, "strace: missing command\n");
    exit(1);
  }


  if(output_file != 0 && !count_summary) {
    close(2);
    if(open(output_file, O_CREATE | O_WRONLY | O_TRUNC) < 0){
      fprintf(2, "strace: can't open %s\n", output_file);
      exit(1);
    }
  }

  if(count_summary) {
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

    getcounts(before);

    trace(0); 

    int pid = fork();
    if(pid == 0){

        int count_mode_mask = mask | (1 << 30);
        trace(count_mode_mask);
        
        exec(argv[i], &argv[i]);
        fprintf(2, "strace: exec %s failed\n", argv[i]);
        exit(1);
    }
    wait(0);

    getcounts(after);


    if(output_file != 0) {
      close(2);
      if(open(output_file, O_CREATE | O_WRONLY | O_TRUNC) < 0){
        fprintf(2, "strace: can't open %s\n", output_file);
        exit(1);
      }
    }


    fprintf(2, "syscall              count\n");
    fprintf(2, "----------------------------\n");
    for(int idx = 1; idx < 24; idx++){
        int diff = after[idx] - before[idx];
        if(diff > 0)
            fprintf(2, "%s: %d\n", sysnames[idx], diff);
    }
    exit(0);
  } 
  
 
  trace(mask);
  if(exec(argv[i], &argv[i]) < 0){
    fprintf(2, "strace: exec %s failed\n", argv[i]);
  }
  
  exit(0);
}
