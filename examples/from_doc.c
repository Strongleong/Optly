#define OPTLY_IMPLEMENTATION
#include "optly.h"
#include <stdio.h>

// This file mirrors the documented "manual struct" configuration style.
static OptlyCommand cmd = {
  // If NULL, parser will set this to argv[0].
  .name = NULL,
  .description = "Example copied from docs with extra comments",

  // Global flags must appear before command tokens.
  .flags = (OptlyFlag[]){
    {"verbose", 'v', "Enable verbose output", .value.as_bool = false, .type = OPTLY_TYPE_BOOL},
    {"threads", 't', "Worker threads", false, false, {.as_uint32 = 4}, OPTLY_TYPE_UINT32},
    NULL_FLAG,
  },

  .commands = (OptlyCommand[]){
    // Same as: app run
    optly_command(
      "run", "Run server",

      // Command-local flags: app run -p 8080
      .flags = optly_flags(
        optly_flag_uint16("port", 'p', "Server port", .value.as_uint16 = 8080),
        optly_flag_bool("verbose", 'v', "Verbose output for run command", .value.as_bool = false)
      ),

      // Same as: app run check / app run dump-config
      .commands = optly_commands(
        optly_command("check", "Health check"),
        optly_command("dump-config", .flags = optly_flags(optly_flag_bool("color", 'c', "Enable colors")))
      ),

      // Optional address positional for "run" command.
      .positionals = optly_positionals(optly_positional("address", "Address to listen on", .min = 0, .max = 1))
    ),
    NULL_COMMAND,
  }
};

int main(int argc, char **argv) {
  OptlyErrors errs = optly_parse_args(argc, argv, &cmd);
  if (errs.count > 0) return 1;

  printf("global.verbose = %s\n", optly_flag_value_bool(&cmd, "verbose") ? "true" : "false");
  printf("global.threads = %u\n", optly_flag_value_uint32(&cmd, "threads"));

  if (!cmd.next_command) {
    return 0;
  }

  printf("command        = %s\n", cmd.next_command->name);
  printf("run.verbose    = %s\n", optly_flag_value_bool(cmd.next_command, "verbose") ? "true" : "false");
  printf("run.port       = %u\n", optly_flag_value_uint16(cmd.next_command, "port"));

  if (cmd.next_command->next_command) {
    printf("subcommand     = %s\n", cmd.next_command->next_command->name);
  }

  OptlyPositional *address = optly_get_positional(cmd.next_command, "address");
  if (address && address->count == 1) {
    printf("address        = %s\n", address->values[0]);
  }

  return 0;
}
