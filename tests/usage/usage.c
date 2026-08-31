// Prints the generated usage for a command tree that exercises flags,
// subcommands and positionals.
#define OPTLY_IMPLEMENTATION
#define OPTLY_GEN_HELP_FLAG
#include "optly.h"

int main(int argc, char **argv) {
  OptlyCommand cmd = {
    .name = "app",
    .description = "Example application",
    .flags = optly_flags(
      optly_flag_bool("verbose", 'v', "Enable verbose output", .value.as_bool = false),
      optly_flag_uint32("threads", 't', "Worker threads", .value.as_uint32 = 4)
    ),
    .commands = optly_commands(
      optly_command("run", "Run server",
        .flags = optly_flags(
          optly_flag_uint16("port", 'p', "Server port", .value.as_uint16 = 8080)
        )
      )
    ),
    .positionals = optly_positionals(
      optly_positional("files", "Files to process", .min = 0, .max = 0)
    )
  };

  optly_parse_args(argc, argv, &cmd);
  optly_usage(&cmd);
  return 0;
}
