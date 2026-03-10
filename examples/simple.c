#include <stdio.h>

#define OPTLYARGS_IMPLEMENTATION
#include <optly.h>

static OptlyFlag flags[] = {
  { "value",  'v', "value",  {0},     OPTLY_TYPE_UINT32 },
  { "switch", 's', "switch", {false}, OPTLY_TYPE_BOOL },
  NULL_FLAG,
};

static OptlyCommand commands[] = {
    {
      "help",
      "see help",
      (OptlyFlag[]){NULL_FLAG},
      (OptlyCommand[]){
        OPTLY_CMD("test", "test cmd", { "flag", 'f', "flag for test cmd", {0}, OPTLY_TYPE_BOOL },),
        NULL_COMMAND,
      },
      NULL,
      {0},
    },

    OPTLY_CMD("download", "get from url",
              {"url", 'u', "url", {NULL}, OPTLY_TYPE_STRING},
              {"switch", 's', "switch", {false}, OPTLY_TYPE_BOOL}, ),

    NULL_COMMAND,
};

int main(int argc, char *argv[]) {
  OptlyCommand cmd = optly_parse_args(argc, argv, flags, commands);

  if (flags[2].value.as_bool || &cmd == &commands[0]) {
    optly_usage(cmd.name, cmd.commands, cmd.flags);
    return 0;
  }

  // printf("Binary: %s\n", cmd.bin_path);

  optly_command_usage("a", &cmd);
  // if (cmd == &commands[1]) {
  //   // optly_usage(cmd.bin_path, commands, flags);
  // }

  printf("Command: %s\n", cmd.name);

  printf("Value  = %u\n", flags[0].value.as_uint32);
  printf("Switch = %s\n\n", flags[1].value.as_bool ? "true" : "false");

  printf("Download url    = %s\n", commands[1].flags[0].value.as_string);
  printf("Download switch = %s\n", commands[1].flags[1].value.as_bool ? "true" : "false");

  printf("Positionals: ");

  // for (size_t i = 0; i < cmd.positionals.count - 1; i++) {
  //   printf("%s, ", cmd.positionals.values[i]);
  // }
  //
  // printf("%s\n", cmd.positionals.values[cmd.positionals.count - 1]);
  return 0;
}
