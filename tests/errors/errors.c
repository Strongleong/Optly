// Prints every accumulated error in order. OPTLY_NO_EXIT keeps the process
// alive so the whole list is observable; tests/exit_behaviour covers the
// default, which is to exit.
#define OPTLY_NO_EXIT
#define OPTLY_IMPLEMENTATION
#include "optly.h"
#include <stdio.h>

int main(int argc, char **argv) {
  OptlyCommand cmd = {
    .name  = "app",
    .flags = optly_flags(
      optly_flag_string("token", 'T', "Token", .required = true),
      optly_flag_uint32("threads", 't', "Threads", .value.as_uint32 = 4),
      optly_flag_bool("verbose", 'v', "Verbose", .value.as_bool = false)
    ),
    .commands = optly_commands(
      optly_command("run", "Run")
    ),
    .positionals = optly_positionals(
      optly_positional("files", "Files", .min = 1, .max = 2)
    )
  };

  OptlyErrors errs = optly_parse_args(argc, argv, &cmd);

  printf("errors=%zu\n", optly_errors_count(&errs));
  for (size_t i = 0; i < optly_errors_count(&errs); i++) {
    OptlyError e = optly_errors_at(&errs, i);
    printf("%zu: %s (%s)\n", i, optly_error_message(e.kind), e.arg ? e.arg : "");
  }
  return 0;
}
