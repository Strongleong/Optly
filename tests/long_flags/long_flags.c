// A flag name longer than OPTLY_FLAG_BUFFER_LENGTH is truncated into the
// stack copy used to split '='. The copy then has no '=' in it even though
// the argument did.
#define OPTLY_NO_EXIT
#define OPTLY_IMPLEMENTATION
#include "optly.h"
#include <stdio.h>

int main(int argc, char **argv) {
  OptlyCommand cmd = {
    .name  = "app",
    .flags = optly_flags(
      optly_flag_string("out", 'o', "Out", .value.as_string = "a.out")
    )
  };

  OptlyErrors errs = optly_parse_args(argc, argv, &cmd);

  printf("errors=%zu\n", optly_errors_count(&errs));
  printf("out=%s\n", optly_flag_value_string(&cmd, "out"));
  return 0;
}
