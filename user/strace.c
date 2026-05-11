#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define TRACE_ALL 0x7fffffff

int
main(int argc, char *argv[])
{
  int mask;
  int start;

  if(argc < 2){
    fprintf(2, "Usage: strace [-a | mask] command [args...]\n");
    exit(1);
  }

  if(strcmp(argv[1], "-a") == 0){
    mask = TRACE_ALL;
    start = 2;
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

  exec(argv[start], &argv[start]);

  fprintf(2, "strace: exec %s failed\n", argv[start]);
  exit(1);
}
