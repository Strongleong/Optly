// optly exits on a bad command line by default. The C suite cannot observe
// that, because it defines OPTLY_NO_EXIT to stay alive.
#define OPTLY_IMPLEMENTATION
#include "optly.h"
#include <stdio.h>

int main(int argc, char **argv) {
  OptlyCommand cmd = {
    .name  = "app",
    .flags = optly_flags(
      optly_flag_uint32("threads", 't', "Worker threads", .value.as_uint32 = 4)
    )
  };

  optly_parse_args(argc, argv, &cmd);
  printf("survived\n");
  return 0;
}
