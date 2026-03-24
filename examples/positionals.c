#include <stdio.h>

#define OPTLY_IMPLEMENTATION
#include "optly.h"

// Example showing positional assignment rules:
// - first positional requires exactly one
// - second is variadic (max=0)
// - third requires 2..3
// - fourth is optional (0..1)
static OptlyCommand cmd = {
  .name = NULL,
  .description = "Positional routing demo",

  .positionals = optly_positionals(
    optly_positional("exactly-one", .min = 1, .max = 1),
    optly_positional("many", .min = 1, .max = 0),
    optly_positional("two-or-three", .min = 2, .max = 3),
    optly_positional("optional", .min = 0, .max = 1)
  )
};

int main(int argc, char **argv) {
  OptlyErrors errs = optly_parse_args(argc, argv, &cmd);

  if (errs.count > 0) {
    for (size_t i = 0; i < errs.count; i++) {
      fprintf(stderr, "ERR: %s (%s)\n", optly_error_message(errs.items[i].kind), errs.items[i].arg ? errs.items[i].arg : "-");
    }
    return 1;
  }

  for (OptlyPositional *p = cmd.positionals; p->name; p++) {
    printf("%s (%zu values):\n", p->name, p->count);

    for (size_t i = 0; i < p->count; i++) {
      printf("  - %s\n", p->values[i]);
    }
  }

  return 0;
}
