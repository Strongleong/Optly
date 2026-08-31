// A three-level command tree. Prints which command was selected and the flag
// values at each level, so command-local flags are visibly distinct from the
// global ones that share their name.
#define OPTLY_IMPLEMENTATION
#include "optly.h"
#include <stdio.h>

int main(int argc, char **argv) {
  OptlyCommand cmd = {
    .name  = "app",
    .flags = optly_flags(
      optly_flag_bool("verbose", 'v', "Verbose", .value.as_bool = false)
    ),
    .commands = optly_commands(
      optly_command("run", "Run",
        .flags = optly_flags(
          optly_flag_uint16("port", 'p', "Port", .value.as_uint16 = 8080),
          optly_flag_bool("verbose", 'v', "Verbose", .value.as_bool = false)
        ),
        .commands = optly_commands(
          optly_command("check", "Check",
            .flags = optly_flags(
              optly_flag_bool("strict", 's', "Strict", .value.as_bool = false)
            )
          )
        )
      ),
      optly_command("build", "Build")
    )
  };

  optly_parse_args(argc, argv, &cmd);

  printf("global.verbose=%d\n", optly_flag_value_bool(&cmd, "verbose"));

  OptlyCommand *c = cmd.next_command;
  if (!c) {
    printf("command=<none>\n");
    return 0;
  }

  printf("command=%s\n", c->name);

  if (optly_is_command(c, "run")) {
    printf("run.port=%u\n", optly_flag_value_uint16(c, "port"));
    printf("run.verbose=%d\n", optly_flag_value_bool(c, "verbose"));
  }

  if (c->next_command) {
    printf("subcommand=%s\n", c->next_command->name);
    printf("check.strict=%d\n", optly_flag_value_bool(c->next_command, "strict"));
  }

  return 0;
}
