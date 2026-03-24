#include <stdio.h>

#define OPTLY_GEN_HELP_FLAG
#define OPTLY_IMPLEMENTATION
#include "optly.h"

static OptlyCommand cmd = {
  .name        = NULL,
  .description = NULL,

  .flags = optly_flags(
    // First value in array is default value, the rest of them are possible values.
    // You need to manually cast to (char *[]) beaause it is C :(
    // Also don't forget to close the values array with NULL
    optly_flag_enum("level", 'l', .value.as_enum = (char *[]){"info", "verb", "info", "warn", NULL}),

    // You can ues `optly_enum_values` to not manually cast into char*[] and apeend NULL to the end
    optly_flag_enum("enum", 'e', optly_enum_values(NULL, "a", "b", "c"), .required = true)
  ),
};

int main(int argc, char **argv) {
  optly_parse_args(argc, argv, &cmd);

  // You can get the value with handy accessor function, as always
  printf("level %s\n", optly_flag_value_enum(&cmd, "level"));

  // Or by hand.
  const OptlyFlag *enumm = optly_get_flag(cmd.flags, "enum");
  printf("enum %s", enumm->value.as_enum[0]);

  // Enum values are still intact and you can use them as you want
  printf(", but it could be %s, %s or even %s :P\n", enumm->value.as_enum[1], enumm->value.as_enum[2], enumm->value.as_enum[3]);

  return 0;
}
