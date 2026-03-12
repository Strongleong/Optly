#define OPTLY_IMPLEMENTATION
#include "optly.h"
static OptlyCommand cmd = {
  .name = NULL,
  .description = NULL,

  .positionals = optly_positionals(
    optly_positional("exactly one", .min = 1, .max = 1),
    optly_positional("as many as possilbe", .min = 1, .max = 0),
    optly_positional("two or three", .min = 2, .max = 3),
    optly_positional("optlional", .min = 0, .max = 1)
  )
};

int main(int argc, char **argv) {
  optly_parse_args(argc, argv, &cmd);

  for (OptlyPositional *p = cmd.positionals; p->name; p++) {
    printf("%s:\n", p->name);

    for (size_t i = 0; i < p->count; i++) {
      printf("  %s\n", p->values[i]);
    }
  }

  return 0;
}
