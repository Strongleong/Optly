// The generated help and version flags can have their short forms moved.
// Usage has to advertise the same forms the parser accepts.
#define OPTLY_GEN_HELP_FLAG
#define OPTLY_GEN_VERSION_FLAG
#define OPTLY_HELP_SHORT_FLAG    "-?"
#define OPTLY_VERSION_SHORT_FLAG "-V"
#define OPTLY_IMPLEMENTATION
#include "optly.h"

int main(int argc, char **argv) {
  OptlyCommand cmd = {
    .name  = "app",
    .flags = optly_flags(
      optly_flag_bool("verbose", 'v', "Verbose", .value.as_bool = false)
    )
  };

  optly_parse_args(argc, argv, &cmd, "1.0.0");
  optly_usage(&cmd);
  return 0;
}
