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
    optly_flag_enum("level", 'l', .value.as_enum = (char *[]){"info", "verb", "info", "warn", NULL}),

    // // You can ues `optly_enum_values` to not manually cast into char*[]
    // optly_flag_enum("enum", 'e', optly_enum_values(NULL, "a", "b", "c"), .required = true),

    optly_flag_enum("enum", 'e', .value.as_enum = (char *[]){NULL, "a" , "b", "c", NULL}, .required = true)


  ),
};

int main(int argc, char **argv) {
  OptlyErrors errs = optly_parse_args(argc, argv, &cmd);

  if (errs.count > 0) {
    for (size_t i = 0; i < errs.count; i++) {
      fprintf(stderr, "ERR: %s\n", optly_error_message(errs.items[i].kind));
      return 1;
    }
  }

  printf("level %s\n", optly_flag_value_enum(&cmd, "level"));
  printf("enum %s\n", optly_flag_value_enum(&cmd, "enum"));

  return 0;
}
