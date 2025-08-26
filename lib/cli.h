/*
    cliargs.h - Single-header CLI arguments parser
    STB-style: include this header, then define CLIARGS_IMPLEMENTATION in ONE .c file

    Usage:
        #define CLIARGS_IMPLEMENTATION
        #include "cliargs.h"

    OVERVIEW:
        This library provides a simple command-line argument parser with support
        for commands and flags. Flags can be long-form (--example) or short-form (-e).
        Short-form flags can be batched (e.g., `-abc` = `-a -b -c`).
        Flags can also take values, either separated by space or `=` sign:
            ./app --name=John
            ./app -n John

        Commands are positional arguments that do not start with `-`.
        A command can have its own flags.

    Example:
        `./app --verbose build --target x86`

        In this example:
            * Global flag: --verbose
            * Command: build
            * Command flag: --target x86
*/

#ifndef CLIARGS_H
#define CLIARGS_H

#include <stddef.h>
#include <stdbool.h>

#ifndef CLIDEF
#define CLIDEF
#endif // CLIDEF

// if you need more than 512 args (per ONE command) you should look in the mirror really deep
#define MAX_FLAGS_LENGTH 512
#define MAX_PATH_LENGTH  4096

/**
 * Represents a single CLI flag.
 * If `value` is NULL, it means the flag is present (boolean true).
 */
typedef struct {
    char *fullname;   ///< Long name of the flag (e.g., "verbose")
    char  shortname;  ///< Short name (e.g., 'v')
    char *value;      ///< Optional value for the flag
} CliFlag;

#define NULL_FLAG { NULL, 0, NULL }

/**
 * Represents a CLI command with associated flags.
 */
typedef struct {
    char     *name;                       ///< Command name
    CliFlag  *flags[MAX_FLAGS_LENGTH];    ///< Array of flags termitaed by NULL_FALG macro
} CliCommand;

#define NULL_COMMAND { NULL,  NULL_FLAG  }

/**
 * Represents parsed CLI arguments (binary name, command, flags).
 */
typedef struct {
    char      bin_name[MAX_PATH_LENGTH];           ///< Name of the executable
    CliCommand *command;                           ///< Detected command
    CliFlag   *parsed_flags[MAX_FLAGS_LENGTH];     ///< Array of parsed flags termitaed by NULL_COMMAND macro
    size_t     parsed_flags_count;                 ///< Count of parsed flags
} CliArgs;


/**
 * Parse CLI arguments into a CliArgs structure.
 */
CLIDEF CliArgs cli_parse_args(int argc, char *argv[], CliFlag *flags, CliCommand *commands);

/**
 * Print usage message with commands and global flags.
 */
CLIDEF void cli_usage(char const *const bin_name, CliCommand *commands, CliFlag *flags);


/**
 * Print usage message for a specific command.
 */
CLIDEF void cli_command_usage(char const *const bin_name, CliCommand *command);

#endif // CLIARGS_H

#ifdef CLIARGS_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <libgen.h>

#define SHIFT_ARG(argv, argc) (++(argv), --(argc))

static CLIDEF bool cli_is_flag_null(const CliFlag *flag) {
    return flag == NULL || (flag->fullname == NULL && flag->shortname == 0);
}

static CLIDEF bool cli_is_command_null(const CliCommand *cmd) {
    return cmd == NULL || (cmd->name == NULL && (*cmd->flags == NULL || cli_is_flag_null(*cmd->flags)));
}


/**
 * Print list of available commands.
 */
static CLIDEF void cli__usage_commands_list(CliCommand *commands) {
    fprintf(stderr, "COMMANDS\n");

    for (CliCommand *cmd = commands; !cli_is_command_null(cmd); cmd++) {
        fprintf(stderr, "  %s\n", cmd->name);
    }
}

/**
 * Print list of flags.
 */
static CLIDEF void cli__usage_flags(CliFlag **flags) {
    fprintf(stderr, "FLAGS\n");

    for (CliFlag *flag = *flags; !cli_is_flag_null(flag); flag++) {
        fprintf(stderr, "  -%c --%s\n", flag->shortname, flag->fullname);
    }
}

CLIDEF void cli_command_usage(char const *const bin_name, CliCommand *command) {
  char name[strlen(bin_name) + 1];
  strcpy(name, bin_name);
  fprintf(stderr, "%s [GLOBAL FLAGS] %s [COMMAND FLAGS]\n", basename(name), command->name);
  cli__usage_flags(command->flags);
}

CLIDEF void cli_usage(char const *const bin_name, CliCommand *commands, CliFlag *flags) {
  fprintf(stderr, "%s [GLOBAL FLAGS] <COMMAND> [COMMAND FLAGS]\n", bin_name);
  cli__usage_commands_list(commands);
  cli__usage_flags(&flags);
}

/**
 * Check if argument matches a flag definition.
 */
static CLIDEF bool cli__flag_matches(const char *arg, const CliFlag *flag) {
    bool is_short = arg[1] != '-';

    return (!is_short && strcmp(arg + 2, flag->fullname) == 0) ||
           (is_short && (arg[1] == flag->shortname));
}

/**
 * Find a flag by argument.
 */
static CLIDEF CliFlag *cli__find_flag(const char *arg, CliFlag *flags) {
    for (CliFlag *f = flags; !cli_is_flag_null(f); f++) {
        if (cli__flag_matches(arg, f)) {
            return f;
        }
    }

    return NULL;
}

/**
 * Add a parsed flag to CliArgs.
 */
static CLIDEF void cli__add_flag(CliArgs *args, CliFlag *flag) {
    args->parsed_flags[args->parsed_flags_count++] = flag;
    args->parsed_flags[args->parsed_flags_count]   = NULL;
}

/**
 * Parse flags from argv.
 */
static CLIDEF void cli__parse_flags(char ***argv_ptr, int *argc_ptr, CliArgs *args, CliFlag *flags) {
    char **argv = *argv_ptr;
    int    argc = *argc_ptr;

    if (!argv || argc <= 0 || !*argv) {
        return;
    }

    char *arg = *argv;
    bool is_batch_short = arg[1] != '-' && strlen(arg) > 2;

    if (is_batch_short) {
        for (char *c = &arg[1]; *c; c++) {
            char sarg[3];
            snprintf(sarg, sizeof(sarg), "-%c", *c);

            CliFlag *flag = cli__find_flag(sarg, flags);

            if (!flag) {
                fprintf(stderr, "Unknown short flag: %s\n", sarg);
                continue;
            }

            flag->value = NULL;
            cli__add_flag(args, flag);
        }

        return;
    }

    char *eq    = strchr(arg, '=');
    char *value = NULL;

    if (eq) {
        *eq   = '\0';
        value = eq + 1;
    }

    CliFlag *flag = cli__find_flag(arg, flags);

    if (!flag) {
        fprintf(stderr, "Unknown flag: %s\n", arg);
        return;
    }

    if (!value && argc > 1 && argv[1][0] != '-') {
        value = argv[1];

        SHIFT_ARG(argv, argc);
    }

    flag->value = value;

    cli__add_flag(args, flag);

    *argv_ptr = argv;
    *argc_ptr = argc;
}

/**
 * Parse a command from argv.
 */
static CLIDEF void cli__parse_command(const char *arg, CliArgs *args, CliCommand *commands) {
    for (CliCommand *cmd = commands; cmd != NULL; cmd++) {
        if (strcmp(arg, cmd->name) == 0) {
            args->command = cmd;
            return;
        }
    }
}

/**
 * Parse CLI arguments.
 */
CLIDEF CliArgs cli_parse_args(int argc, char *argv[], CliFlag *flags, CliCommand *commands) {
    assert(argc > 0);

    CliArgs args = {0};
    strcpy(args.bin_name, argv[0]);
    SHIFT_ARG(argv, argc);

    while (argc > 0) {
        char *arg = *argv;

        if (!arg) {
            break;
        }

        if (arg[0] == '-') {
            cli__parse_flags(&argv, &argc, &args, flags);
        } else {
            cli__parse_command(arg, &args, commands);
        }

        SHIFT_ARG(argv, argc);
    }

    return args;
}

#endif // CLIARGS_IMPLEMENTATION

// TODO: Add sub-commands
