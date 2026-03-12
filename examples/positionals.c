#define OPTLY_IMPLEMENTATION
#include "optly.h"
static OptlyCommand cmd = {
  .name = NULL,
  .description = NULL,

  .positionals = optly_positionals(
    optly_positional("source", .min = 1, .max = 0),
    optly_positional("dest", .min = 1, .max = 1)
  )
};

int main(int argc, char **argv) {
  optly_parse_args(argc, argv, &cmd);

  for (size_t i = 0; i < cmd.positionals->count; i++) {
    printf("%s\n", cmd.positionals->values[i]);
  }

  return 0;
}
