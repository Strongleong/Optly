/*
    cliargs.h - Single-header CLI arguments parser
    STB-style: include this header, then define CLIARGS_IMPLEMENTATION in ONE .c file

    Usage:
        #define CLIARGS_IMPLEMENTATION
        #include "cliargs.h"

    Commands:
        Commands are CLI argumets // TODO: TBD

    Flags:
        Flags are CLI arguments that starts with `-`. They can be long form `--example`
        or short form `-s`. Short from flags can be "batched" (`-abcd` is same as `-a -b -c -d`)
        Flags can have parameters that separated from flag with space or equeal sign `=`.
        Batched flags can not have arguments. If argument does not takes argumet then actual value
        of parsed flag is TRUE if flag was found.
        Examples:
            `./app --help`
            `./app --url https://example.com`
            `./app -j=12`
            `./app -lah`

*/

#ifndef CLIARGS_H
#define CLIARGS_H

#include <stddef.h>
#include <stdbool.h>

// if you need more than 512 args for ONE command you should look in the mirror really deep
#define COMMAND_FLAGS_MAX_COUNT 512
#define MAX_PATH_LENGTH 4096

typedef struct {
    char *fullname;
    char shortname;
    char *value;  // If NULL → treated as boolean TRUE
} CliFlag;

typedef struct {
    char *name;
    CliFlag *flags;  // NULL-terminated array of flags
} CliCommand;

typedef struct {
    char bin_name[MAX_PATH_LENGTH];
    CliCommand *command;
    CliFlag **parsed_flags; // dynamically allocated array of pointers to flags
    size_t parsed_flags_count;
} CliArgs;

// Public API
CliArgs *cli_parse_args(int argc, char *argv[], CliFlag *global_flags, CliCommand *commands);
void usage(char const *const bin_name, CliCommand *commands, CliFlag *flags);
void command_usage(char const *const bin_name, CliCommand *command);

// Internal helpers (inline)
static inline bool cli_is_flag_null(const CliFlag *flag) {
    return flag == NULL || (flag->fullname == NULL && flag->shortname == 0);
}

static inline bool cli_is_command_null(const CliCommand *cmd) {
    return cmd == NULL || (cmd->name == NULL && (cmd->flags == NULL || cli_is_flag_null(cmd->flags)));
}

#endif // CLIARGS_H

#ifdef CLIARGS_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define SHIFT_ARG(argv, argc) (++(argv), --(argc))

void usage_commands_list(CliCommand *commands) {
  fprintf(stderr, "COMMANDS\n");

  for (CliCommand *command = commands; !cli_is_command_null(command); command++) {
    fprintf(stderr, "  %s\n", command->name);
  }
}

static void usage_flags(CliFlag *flags) {
  fprintf(stderr, "GLOBAL FLAGS\n");

  for (CliFlag *flag = flags; !cli_is_flag_null(flag); flag++) {
    fprintf(stderr, "  -%c --%s\n", flag->shortname, flag->fullname);
  }
}

void command_usage(char const *const bin_name, CliCommand *command) {
  fprintf(stderr, "%s [GLOBAL FLAGS] %s [COMMAND FLAGS]\n", bin_name, command->name);
  usage_flags(command->flags);
}

void usage(char const *const bin_name, CliCommand *commands, CliFlag *flags) {
  fprintf(stderr, "%s [GLOBAL FLAGS] <COMMAND> [COMMAND FLAGS]\n", bin_name);
  usage_commands_list(commands);
  usage_flags(flags);
}

static bool cli_flag_matches(const char *arg, const CliFlag *flag) {
    bool is_short = arg[1] != '-';
    return (!is_short && strcmp(arg + 2, flag->fullname) == 0) ||
           (is_short && (arg[1] == flag->shortname));
}

static CliFlag *cli_find_flag(const char *arg, CliFlag *flags) {
    for (CliFlag *f = flags; !cli_is_flag_null(f); f++) {
        if (cli_flag_matches(arg, f)) {
            return f;
        }
    }
    return NULL;
}

static void cli_add_flag(CliArgs *args, CliFlag *flag) {
    args->parsed_flags = (CliFlag **)realloc(args->parsed_flags, sizeof(CliFlag *) * (args->parsed_flags_count + 2));
    args->parsed_flags[args->parsed_flags_count++] = flag;
    args->parsed_flags[args->parsed_flags_count] = NULL;
}

static void cli_parse_flags(char ***argv_ptr, int *argc_ptr, CliArgs *args, CliFlag *flags) {
    char **argv = *argv_ptr;
    int argc = *argc_ptr;
    if (!argv || argc <= 0 || !*argv) return;
    char *arg = *argv;

    bool is_batch_short = arg[1] != '-' && strlen(arg) > 2;
    if (is_batch_short) {
        for (char *c = &arg[1]; *c; c++) {
            char sarg[3];
            snprintf(sarg, sizeof(sarg), "-%c", *c);
            CliFlag *flag = cli_find_flag(sarg, flags);
            if (!flag) {
                fprintf(stderr, "Unknown short flag: %s\n", sarg);
                continue;
            }
            flag->value = NULL;
            cli_add_flag(args, flag);
        }
        return;
    }

    char *eq = strchr(arg, '=');
    char *value = NULL;
    if (eq) {
        *eq = '\0';
        value = eq + 1;
    }

    CliFlag *flag = cli_find_flag(arg, flags);
    if (!flag) {
        fprintf(stderr, "Unknown flag: %s\n", arg);
        return;
    }

    if (!value && argc > 1 && argv[1][0] != '-') {
        value = argv[1];
        SHIFT_ARG(argv, argc);
    }

    flag->value = value;
    cli_add_flag(args, flag);

    *argv_ptr = argv;
    *argc_ptr = argc;
}

static void cli_parse_command(const char *arg, CliArgs *args, CliCommand *commands) {
    for (CliCommand *cmd = commands; !cli_is_command_null(cmd); cmd++) {
        if (strcmp(arg, cmd->name) == 0) {
            args->command = cmd;
            return;
        }
    }
}

CliArgs *cli_parse_args(int argc, char *argv[], CliFlag *global_flags, CliCommand *commands) {
    assert(argc > 0);

    CliArgs *args = (CliArgs *)calloc(1, sizeof(CliArgs));
    strcpy(args->bin_name, argv[0]);

    SHIFT_ARG(argv, argc);

    while (argc > 0) {
        char *arg = *argv;

        if (!arg) break;

        if (arg[0] == '-') {
            cli_parse_flags(&argv, &argc, args, global_flags);
        } else {
            cli_parse_command(arg, args, commands);
        }

        SHIFT_ARG(argv, argc);
    }

    return args;
}

#endif // CLIARGS_IMPLEMENTATION
