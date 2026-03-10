#include <stdio.h>

#define OPTLYARGS_IMPLEMENTATION
#include <optly.h>

static OptlyFlag flags[] = {
  { "value",  'v', "value",  {0},     OPTLY_TYPE_UINT32 },
  { "switch", 's', "switch", {false}, OPTLY_TYPE_BOOL },
  NULL_FLAG,
};

static OptlyCommand commands[] = {
  OPTLY_CMD("help", "help"),

  OPTLY_CMD("download", "get from url",
    { "url",    'u', "url",    {NULL},  OPTLY_TYPE_STRING },
    { "switch", 's', "switch", {false}, OPTLY_TYPE_BOOL },
  ),

  NULL_COMMAND,
};

int main(int argc, char *argv[]) {
  OptlyArgs args = optly_parse_args(argc, argv, flags, commands);
  // optly_command_usage(args.bin_path, &commands[1]);
  //
  // for (size_t i = 0; i < args.positionals.count; i++) {
  //   printf("%s\n", args.positionals.values[i]);
  // }
  //
  // return 0;

  if (flags[2].value.as_bool || args.command == &commands[0]) {
    optly_usage(args.bin_path, commands, flags);
    return 0;
  }

  printf("Binary: %s\n", args.bin_path);

  if (args.command) {
    if (args.command == &commands[1]) {
      optly_usage(args.bin_path, commands, flags);
    }

    printf("Command: %s\n", args.command->name);
  }

  printf("Value  = %u\n", flags[0].value.as_uint32);
  printf("Switch = %s\n\n", flags[1].value.as_bool ? "true" : "false");

  printf("Download url    = %s\n", commands[1].flags[0].value.as_string);
  printf("Download switch = %s\n", commands[1].flags[1].value.as_bool ? "true" : "false");

  return 0;
}
