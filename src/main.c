#include <cli.h>
#include <stdio.h>

typedef struct OrbitaDlConfig {
  char const *const domain;
} OrbitaDlConfig;

/* static OrbitaDlConfig defaut_config = { */
/*   .domain = "sasflix.ru" */
/* }; */

static CliFlag global_flags[] = {
  { "help",    'h', NULL },
  { "version", 'v', NULL },
  { "count",   'c', NULL },
  NULL_FLAG,
};

static CliFlag download_flags[] = {
  { "url", 'u' , NULL },
  NULL_FLAG,
};

static CliCommand commands[] = {
  { "download", {download_flags} },
  NULL_COMMAND,
};

int main(int argc, char *argv[]) {
  CliArgs args = cli_parse_args(argc, argv, global_flags, commands);
  cli_usage(args.bin_name, commands, global_flags);

  printf("Binary: %s\n", args.bin_name);

  if (args.command) {
    printf("Command: %s\n", args.command->name);
  }

  for (size_t i = 0; i < args.parsed_flags_count; i++) {
    CliFlag *f = args.parsed_flags[i];
    printf("Flag: --%s (-%c) = %s\n", f->fullname, f->shortname, f->value ? f->value : "TRUE");
  }

  return 0;
}
