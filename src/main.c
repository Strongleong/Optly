#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

 // if you need more than 512 args for ONE command you should look in the mirror really deep
#define COMMAND_FLAGS_MAX_COUNT 512

typedef struct OrbitaDlConfig {
  const char *const domain;
} OrbitaDlConfig;

/* static OrbitaDlConfig defaut_config = { */
/*   .domain = "sasflix.ru" */
/* }; */

typedef struct Flag {
  const char *const fullname;
  const char shortname;
} Flag;

#define NULL_FLAG { NULL, 0 }

inline bool is_flag_null(Flag *flag) {
  return flag->fullname == NULL && flag->shortname == 0;
}

static Flag global_flags[] = {
  { "help",    'h' },
  { "version", 'v' },
  NULL_FLAG
};

typedef struct Command {
  const char *const name;
  Flag flags[COMMAND_FLAGS_MAX_COUNT];
} Command;

#define NULL_COMMAND { NULL, NULL_FLAG }

inline bool is_command_null(Command *command) {
  return command->name == NULL && is_flag_null(command->flags);
}

typedef struct Args {
  const char *const bin_name;
  Command command;
  Flag *flags;
} Args;

static Command commands[] = {
  {
    .name = "download",
    .flags = { { "url", 'u' }, NULL_FLAG }
  },
  NULL_COMMAND
};

void usage_commands_list() {
  fprintf(stderr, "COMMANDS\n");

  for (Command *command = commands; !is_command_null(command); command++) {
    fprintf(stderr, "  %s\n", commands->name);
  }
}

void usage_flags(Flag *flags) {
  fprintf(stderr, "GLOBAL FLAGS\n");

  for (Flag *flag = flags; !is_flag_null(flag); flag++) {
    fprintf(stderr, "  -%c --%s\n", flag->shortname, flag->fullname);
  }
}

void command_help(Command *command) {
  fprintf(stderr, "orbitadl [GLOBAL FLAGS] %s [COMMAND FLAGS]\n", command->name);
  usage_flags(command->flags);
}

void usage() {
  fprintf(stderr, "orbitadl [GLOBAL FLAGS] <COMMAND> [COMMAND FLAGS]\n");
  usage_commands_list();
  usage_flags(global_flags);
}

void parse_args(int argc, char *argv[], Args *args) {
  assert(argc > 0);
  char *arg = *argv++; argc++;
  arg = *argv++; argc++;
  /* char *tmp = ""; */

  while (arg && arg[0] == '-') {
    bool is_short = false;

    if (arg[1] == '-') {
      is_short = true;
    }

    memmove(arg, arg + (is_short ? 0 : 1), sizeof(arg) - (is_short ? 0 : 1) );
    printf("%s (0x%02X)\n", arg, arg[0]);

    for (Flag *flag = global_flags; !is_flag_null(flag); flag++) {
      printf("%c (0x%02X)\n", flag->shortname, flag->shortname);
      if (
        (!is_short && strcmp(arg, flag->fullname) == 0) ||
        (is_short && arg[0] == flag->shortname)
      ) {
        printf("Found arg %s", flag->fullname);
        args->flags = flag;
        args->flags++;
        break;
      }

      /* fprintf(stderr, "Unknow argument '%s': skipping\n", arg); */
    }

    /* if (strcmp(arg, flag->fullname) == 0 || strcmp(arg, flag->shortname) == ) { */
    /* } */

    arg = *argv++; argc++;
  }

  /* *(args->flags) = (Flag)NULL_FLAG; */
}

int main(int argc, char *argv[]) {
  Flag flags[COMMAND_FLAGS_MAX_COUNT];
  Args args = {};
  args.flags = flags;

  parse_args(argc, argv, &args);
  usage();

  return 0;
}
