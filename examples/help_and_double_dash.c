#include <stdio.h>

// Enable generated help flag for this sample.
#define OPTLY_GEN_HELP_FLAG
#define OPTLY_NO_EXIT
#define OPTLY_IMPLEMENTATION
#include "optly.h"

static OptlyCommand cmd = {
  .name = "dash-demo",
  .flags = optly_flags(
    optly_flag_bool("verbose", .shortname = 'v')
  ),
  .positionals = optly_positionals(
    optly_positional("args", .min = 0, .max = 0)
  )
};

int main(int argc, char **argv) {
  // Example:
  //   ./dash-demo -v -- --not-a-flag literal
  // After '--', tokens are treated as positionals.
  OptlyErrors errs = optly_parse_args(argc, argv, &cmd);

  if (errs.count > 0) {
    for (size_t i = 0; i < errs.count; i++) {
      fprintf(stderr, "ERR: %s\n", optly_error_message(errs.items[i].kind));
    }
    return 1;
  }

  printf("verbose = %s\n", optly_flag_value_bool(&cmd, "verbose") ? "true" : "false");

  OptlyPositional *args = optly_get_positional(&cmd, "args");
  for (size_t i = 0; args && i < args->count; i++) {
    printf("args[%zu] = %s\n", i, args->values[i]);
  }

  return 0;
}
