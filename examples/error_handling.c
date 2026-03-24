#include <stdio.h>

// Disable parser auto-exit so we can inspect all parse errors ourselves.
#define OPTLY_NO_EXIT
#define OPTLY_IMPLEMENTATION
#include "optly.h"

static OptlyCommand cmd = {
  .name = "err-demo",
  .flags = optly_flags(
    optly_flag_uint16("port", .shortname = 'p', .required = true),
    optly_flag_bool("verbose", .shortname = 'v')
  ),
  .commands = optly_commands(
    optly_command("run", .positionals = optly_positionals(optly_positional("target", .min = 1, .max = 1)))
  )
};

int main(int argc, char **argv) {
  OptlyErrors errs = optly_parse_args(argc, argv, &cmd);

  if (errs.count == 0) {
    puts("No errors.");
    return 0;
  }

  puts("Parse errors:");
  for (size_t i = 0; i < optly_errors_count(&errs); i++) {
    OptlyError e = optly_errors_at(&errs, i);
    printf("- %s", optly_error_message(e.kind));
    if (e.arg) printf(" (%s)", e.arg);
    printf("\n");
  }

  return 1;
}
