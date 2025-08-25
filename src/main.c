#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define SHIFT_ARR(arr, len) *(arr)++; (len)--

// if you need more than 512 args for ONE command you should look in the mirror really deep
#define COMMAND_FLAGS_MAX_COUNT 512
#define MAX_PATH_LENGTH 4096

typedef struct OrbitaDlConfig {
  char const *const domain;
} OrbitaDlConfig;

/* static OrbitaDlConfig defaut_config = { */
/*   .domain = "sasflix.ru" */
/* }; */

typedef struct Flag {
  char *fullname;
  char shortname;
} Flag;

#define NULL_FLAG { NULL, 0 }

inline bool is_flag_null(Flag *flag) {
  return flag->fullname == NULL && flag->shortname == 0;
}

inline bool is_flag(char const *const arg, Flag flag) {
  bool is_short = arg[1] != '-';

  return (!is_short && (strcmp(arg+2, flag.fullname) == 0)) ||
  (is_short && (arg[1] == flag.shortname));
}

Flag parse_flag(char *arg, Flag *flags) {
  for (Flag *flag = flags; !is_flag_null(flag); flag++) {
    if (is_flag(arg, *flag)) {
      return *flag;
    }
  }

  return (Flag)NULL_FLAG;
}

static Flag global_flags[] = {
  { "help",    'h' },
  { "version", 'v' },
  { "ass", 'a' },
  NULL_FLAG
};

typedef struct Command {
  char const *const name;
  Flag flags[COMMAND_FLAGS_MAX_COUNT];
} Command;

#define NULL_COMMAND { NULL, { NULL_FLAG } }

inline bool is_command_null(Command *command) {
  return command->name == NULL && is_flag_null(command->flags);
}

typedef struct Args {
  char bin_name[MAX_PATH_LENGTH];
  Command command;
  Flag *flags;
  size_t flags_count;
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
  char *arg = SHIFT_ARR(argv, argc);

  strcpy(args->bin_name, arg);
  size_t count = 0;

  arg = SHIFT_ARR(argv, argc);

  // TODO: Come up with better way of handling batched short args
  while (arg && arg[0] == '-') {
    bool is_batch_short_args = arg[1] != '-' && strlen(arg) > 2;

    if (is_batch_short_args) {
      for (char *c = &arg[1]; *c != '\0'; c++) {
        char sarg[3];
        sprintf(sarg, "-%c", *c);

        // Copy pased start
        Flag flag = parse_flag(sarg, global_flags);

        if (is_flag_null(&flag)) {
          fprintf(stderr, "Unknow argument '%s': skipping\n", sarg);
        } else {
          args->flags[count] = flag;
          count++;
        }
        // Copy pased end
      }
    } else {
      // Copy pased start
      Flag flag = parse_flag(arg, global_flags);

      if (is_flag_null(&flag)) {
        fprintf(stderr, "Unknow argument '%s': skipping\n", arg);
      } else {
        args->flags[count] = flag;
        count++;
      }
      // Copy pased end
    }

    arg = SHIFT_ARR(argv, argc);
  }

  args->flags[count] = (Flag)NULL_FLAG;
  args->flags_count = count;
}

int main(int argc, char *argv[]) {
  Flag flags[COMMAND_FLAGS_MAX_COUNT] = {0};
  Args args = {0};
  args.flags = flags;

  parse_args(argc, argv, &args);
  /* usage(); */

  printf("bin_name = %s\n", args.bin_name);
  printf("flags_count = %zu\n", args.flags_count);

  for (Flag *flag = flags; !is_flag_null(flag); flag++) {
    printf("flag: %s\n", flag->fullname);
  }

  return 0;
}
