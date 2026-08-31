// Enum flags carry their allowed values inline; slot 0 holds the current one.
#define OPTLY_NO_EXIT
#define OPTLY_IMPLEMENTATION
#include "optly.h"
#include <stdio.h>

int main(int argc, char **argv) {
  OptlyCommand cmd = {
    .name  = "app",
    .flags = optly_flags(
      optly_flag_enum("log", 'l', "Log level", optly_enum_values("verbose", "debug", "verbose", "warn")),
      optly_flag_bool("force", 'f', "Force", .value.as_bool = false)
    )
  };

  OptlyErrors errs = optly_parse_args(argc, argv, &cmd);

  printf("errors=%zu\n", optly_errors_count(&errs));
  for (size_t i = 0; i < optly_errors_count(&errs); i++) {
    OptlyError e = optly_errors_at(&errs, i);
    printf("%zu: %s (%s)\n", i, optly_error_message(e.kind), e.arg ? e.arg : "");
  }
  printf("log=%s\n", optly_flag_value_enum(&cmd, "log"));
  printf("force=%d\n", optly_flag_value_bool(&cmd, "force"));
  return 0;
}
