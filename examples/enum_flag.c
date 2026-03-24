#include <stdio.h>

// Enable optional help flag and implementation in this translation unit.
#define OPTLY_GEN_HELP_FLAG
#define OPTLY_IMPLEMENTATION
#include "optly.h"

// Enum flag layout:
//   value.as_enum[0] = currently selected value (default + parse target)
//   value.as_enum[1..] = allowed values (NULL-terminated)
static OptlyCommand cmd = {
  .name = NULL,

  .flags = optly_flags(
    // Manual enum storage format.
    optly_flag_enum("level", 'l', .value.as_enum = (char *[]){"info", "debug", "info", "warn", "error", NULL}),

    // Helper macro format (same result, less boilerplate).
    optly_flag_enum("format", 'f', optly_enum_values("text", "text", "json", "pretty"), .required = true)
  ),
};

int main(int argc, char **argv) {
  // Parse and collect errors.
  OptlyErrors errs = optly_parse_args(argc, argv, &cmd);

  if (errs.count > 0) {
    for (size_t i = 0; i < errs.count; i++) {
      fprintf(stderr, "ERR: %s", optly_error_message(errs.items[i].kind));

      if (errs.items[i].arg) {
        fprintf(stderr, " (%s)", errs.items[i].arg);
      }

      fprintf(stderr, "\n");
    }

    return 1;
  }

  // Access chosen enum values with helper accessor.
  printf("level  = %s\n", optly_flag_value_enum(&cmd, "level"));
  printf("format = %s\n", optly_flag_value_enum(&cmd, "format"));

  // Allowed values remain available in the same array.
  const OptlyFlag *format = optly_get_flag(cmd.flags, "format");
  printf("format options: %s, %s, %s\n", format->value.as_enum[1], format->value.as_enum[2], format->value.as_enum[3]);

  return 0;
}
