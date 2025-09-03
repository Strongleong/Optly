#include <optly.h>
#include <stdio.h>

static OptlyFlag global_flags[] = {
  { "help",    'h', {false}, OPTLY_TYPE_BOOL },
  { "version", 'v', {false}, OPTLY_TYPE_BOOL },
  { "ass",     'a', {NULL},  OPTLY_TYPE_UINT8 },
  NULL_FLAG,
};

static OptlyFlag download_flags[] = {
  { "url", 'u' , {NULL}, OPTLY_TYPE_STRING },
  NULL_FLAG,
};

static OptlyCommand commands[] = {
  { "help",  { NULL }  },
  { "download", {download_flags} },
  NULL_COMMAND,
};

int main(int argc, char *argv[]) {
  OptlyArgs args = optly_parse_args(argc, argv, global_flags, commands);

  /* optly_usage(args.bin_path, commands, global_flags); */

  printf("Binary: %s\n", args.bin_path);

  if (args.command) {
    printf("Command: %s\n", args.command->name);
  }

  for (OptlyFlag *f = global_flags; !optly_is_flag_null(f); f++) {
    printf("Flag: --%s (-%c) = %s\n", f->fullname, f->shortname, f->value.as_bool ? "true" : "false");
  }

  return 0;
}
