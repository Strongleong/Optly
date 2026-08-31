// Dumps every flag optly parsed, so the .tspec can assert on exact bytes.
#define OPTLY_IMPLEMENTATION
#include "optly.h"
#include <stdio.h>

int main(int argc, char **argv) {
  OptlyCommand cmd = {
    .name  = "app",
    .flags = optly_flags(
      optly_flag_bool("verbose", 'v', "Verbose", .value.as_bool = false),
      optly_flag_bool("quiet", 'q', "Quiet", .value.as_bool = false),
      optly_flag_bool("force", 'f', "Force", .value.as_bool = false),
      optly_flag_uint32("threads", 't', "Threads", .value.as_uint32 = 4),
      optly_flag_string("out", 'o', "Output", .value.as_string = "a.out")
    )
  };

  optly_parse_args(argc, argv, &cmd);

  printf("verbose=%d\n", optly_flag_value_bool(&cmd, "verbose"));
  printf("quiet=%d\n",   optly_flag_value_bool(&cmd, "quiet"));
  printf("force=%d\n",   optly_flag_value_bool(&cmd, "force"));
  printf("threads=%u\n", optly_flag_value_uint32(&cmd, "threads"));
  printf("out=%s\n",     optly_flag_value_string(&cmd, "out"));
  return 0;
}
