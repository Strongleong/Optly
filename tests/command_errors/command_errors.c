// A command tree with no positionals, so an unrecognised token is reported as
// an unknown command instead of being collected as a positional value.
#define OPTLY_NO_EXIT
#define OPTLY_IMPLEMENTATION
#include "optly.h"
#include <stdio.h>

int main(int argc, char **argv) {
  OptlyCommand cmd = {
    .name  = "app",
    .flags = optly_flags(
      optly_flag_bool("verbose", 'v', "Verbose", .value.as_bool = false)
    ),
    .commands = optly_commands(
      optly_command("run", "Run"),
      optly_command("build", "Build")
    )
  };

  OptlyErrors errs = optly_parse_args(argc, argv, &cmd);

  printf("errors=%zu\n", optly_errors_count(&errs));
  for (size_t i = 0; i < optly_errors_count(&errs); i++) {
    OptlyError e = optly_errors_at(&errs, i);
    printf("%zu: %s (%s)\n", i, optly_error_message(e.kind), e.arg ? e.arg : "");
  }
  printf("command=%s\n", cmd.next_command ? cmd.next_command->name : "<none>");
  return 0;
}
