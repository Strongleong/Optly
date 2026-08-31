// Prints every positional value in order, so ordering and the -- delimiter
// are both pinned by the expected bytes.
#define OPTLY_IMPLEMENTATION
#include "optly.h"
#include <stdio.h>

int main(int argc, char **argv) {
  OptlyCommand cmd = {
    .name  = "app",
    .flags = optly_flags(
      optly_flag_bool("verbose", 'v', "Verbose", .value.as_bool = false)
    ),
    .positionals = optly_positionals(
      optly_positional("files", "Files", .min = 1, .max = 0)
    )
  };

  optly_parse_args(argc, argv, &cmd);

  printf("verbose=%d\n", optly_flag_value_bool(&cmd, "verbose"));

  OptlyPositional *p = optly_get_positional(&cmd, "files");
  if (!p) {
    printf("files=<none>\n");
    return 0;
  }

  printf("count=%zu\n", p->count);
  for (size_t i = 0; i < p->count; i++) {
    printf("files[%zu]=%s\n", i, p->values[i]);
  }
  return 0;
}
