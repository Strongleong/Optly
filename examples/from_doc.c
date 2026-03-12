#define OPTLY_IMPLEMENTATION
#include "optly.h"

// You need to define "main" command, which is your application.
static OptlyCommand cmd = {

  // Name will be used in usage. If you set is as NULL it will be argv[0]
  .name = NULL,

  // If not null, then it will be used in usage
  .description = NULL,

  // This is your "top-level global" flags. They should be specified before
  // any command (./app --thread 2 -v)
  .flags =
    (OptlyFlag[]){
      // Long (full name)   short name   description (for usage)  Default
      // value           Flag value  type
      {"verbose", 'v', "Enable verbose output", .value.as_bool = false, .type = OPTLY_TYPE_BOOL},

      // Values are unions, so you need to specify member of value with
      // correct type some way
      {"threads", 't', "Worker threads", false, false, {.as_uint32 = 4}, OPTLY_TYPE_UINT32},

      // Flag arrays should alwasy ends with NULL_FLAG. Try to not forget
      // about it :)
      NULL_FLAG,
    },

  // This is your commands. (git `commit`, docker `compose` `up`)
  .commands = (OptlyCommand[]){

    // Instead of defining whole struct manually you can use helper
    // functions
    optly_command(
      "run", "Runs server",

      // optly_flags macro deals with type castings and closing array with
      // NULL_FLAG
      optly_flags(

        // This is *command* flag. `./app -p 8080 run` will not work,
        // but `./app run -p 8080` will
        optly_flag_uint16("port", 'p', "Server port", .value.as_uint16 = 8080),

        // NOTE: this flag and 'verbose' GLOBAL flag are not the same
        // `./app -v run` - enables global -v flag
        // `./app run -v` - enables command -v flag
        optly_flag_bool("verbose", 'v', "Enable verbose output for worker", .value.as_bool = false)
      ),

      // This is subcommands of command "run"
      optly_commands(

        // `./app run check` subcommand have no description, flags or
        // subcommands.
        //  NULL is here to suppress warning about not providing
        //  argument in variadic macro
        optly_command("check", NULL),

        // `./app run dump_config` subcommands
        optly_command(
          "dump_config",

          // We skipped description, so we need to specify fileds now
          .flags = optly_flags(
            optly_flag_bool("color", 'c', "Show colors", .value.as_bool = false)
          )
        )
      )
    ),
  }
};

int main(int argc, char **argv) {
  optly_parse_args(argc, argv, &cmd);

  printf("Verbose: %s\n", optly_flag_value_bool(&cmd, "verbose") ? "true" : "false");
  printf("Threads: %u\n", optly_flag_value_uint32(&cmd, "threads"));

  if (!cmd.next_command) {
    return 0;
  }

  printf("Command: %s\n", cmd.next_command->name);

  printf("Verbose: %s\n", optly_flag_value_bool(cmd.next_command, "verbose") ? "true" : "false");
  printf("Port: %u\n", optly_flag_value_uint16(cmd.next_command, "port"));

  if (cmd.next_command->next_command) {
    printf("Command: %s\n", cmd.next_command->next_command->name);
  }
}
