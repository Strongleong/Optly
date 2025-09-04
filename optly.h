/*
    optly.h - v1.0.0 - Single-header OPTLY arguments parser
    STB-style: include this header, then define OPTLYARGS_IMPLEMENTATION in ONE .c file

    Usage:
        #define OPTLYARGS_IMPLEMENTATION
        #include "optlyargs.h"

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

#ifndef OPTLYARGS_H
#define OPTLYARGS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef OPTLYDEF
#define OPTLYDEF
#endif // OPTLYDEF

// if you need more than 512 args per ONE command you should look in the mirror really deep
#define MAX_FLAGS_LENGTH 512
#define MAX_PATH_LENGTH  4096

typedef enum OptlyFlagType {
    OPTLY_TYPE_BOOL,
    OPTLY_TYPE_CHAR,
    OPTLY_TYPE_STRING,
    OPTLY_TYPE_INT8,
    OPTLY_TYPE_INT16,
    OPTLY_TYPE_INT32,
    OPTLY_TYPE_INT64,
    OPTLY_TYPE_UINT8,
    OPTLY_TYPE_UINT16,
    OPTLY_TYPE_UINT32,
    OPTLY_TYPE_UINT64,
    OPTLY_TYPE_FLOAT,
    OPTLY_TYPE_DOUBLE,
    /* OPTLY_TYPE_PTR, */
} OptlyFlagType;

typedef union OtplyFlagValue {
    void  *as_ptr;
    bool   as_bool;

    char  as_char;
    char *as_string;

    int8_t  as_int8;
    int16_t as_int16;
    int32_t as_int32;
    int64_t as_int64;

    uint8_t  as_uint8;
    uint16_t as_uint16;
    uint32_t as_uint32;
    uint64_t as_uint64;

    float  as_float;
    double as_double;
} OptlyFlagValue;

/**
 * Represents a single OPTLY flag.
 * If `value` is non-NULL, it means the flag is present (boolean true).
 */
typedef struct {
    char *fullname;   ///< Long name of the flag (e.g., "verbose")
    char  shortname;  ///< Short name (e.g., 'v')
    OptlyFlagValue value; ///< Value for the flag, based on flag type
    OptlyFlagType type; ///< Flag type
} OptlyFlag;

/**
 * Represents a OPTLY command with associated flags.
 */
typedef struct {
    char       *name;                       ///< Command name
    OptlyFlag  *flags[MAX_FLAGS_LENGTH];    ///< Array of flags termitaed by NULL_FALG macro
} OptlyCommand;

typedef struct OptlyArgs {
    OptlyCommand *command;
    char bin_path[MAX_PATH_LENGTH];
} OptlyArgs;

#define NULL_FLAG    { .fullname = NULL, .shortname = 0, .value = { .as_ptr = NULL }, .type = 0 }
#define NULL_COMMAND { .name = NULL, .flags = { NULL } }

/**
 * Parse OPTLY arguments into a OptlyArgs structure.
 */
OPTLYDEF OptlyArgs optly_parse_args(int argc, char *argv[], OptlyFlag *flags, OptlyCommand *commands);

/**
 * Print usage message with commands and global flags.
 */
OPTLYDEF void optly_usage(char const *const bin_path, OptlyCommand *commands, OptlyFlag *flags);

/**
 * Print usage message for a specific command.
 */
OPTLYDEF void optly_command_usage(char const *const bin_path, OptlyCommand *command);

/**
 * Inline helpers
 */

inline bool optly_is_flag_null(const OptlyFlag *flag) {
    return flag == NULL || (flag->fullname == NULL && flag->shortname == 0);
}

inline bool optly_is_command_null(const OptlyCommand *cmd) {
    return cmd == NULL || (cmd->name == NULL && (*cmd->flags == NULL || optly_is_flag_null(*cmd->flags)));
}

// -----------------------------------
// -----------------------------------

#endif // OPTLYARGS_H

#ifdef OPTLYARGS_IMPLEMENTATION
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <libgen.h>
#include <stdlib.h>

#define SHIFT_ARG(argv, argc) (++(argv), --(argc))

/**
 * Print list of available commands.
 */
static OPTLYDEF void optly__usage_commands_list(OptlyCommand *commands) {
    fprintf(stderr, "COMMANDS\n");

    for (OptlyCommand *cmd = commands; !optly_is_command_null(cmd); cmd++) {
        fprintf(stderr, "  %s\n", cmd->name);
    }
}

/**
 * Print list of flags.
 */
static OPTLYDEF void optly__usage_flags(OptlyFlag **flags) {
    fprintf(stderr, "FLAGS\n");

    for (OptlyFlag *flag = *flags; !optly_is_flag_null(flag); flag++) {
        fprintf(stderr, "  -%c --%s\n", flag->shortname, flag->fullname);
    }
}

OPTLYDEF void optly_command_usage(char const *const bin_path, OptlyCommand *command) {
  // NOTE: Copying const path into non-const path bause of libgen basename
  char name[strlen(bin_path) + 1];
  strcpy(name, bin_path);
  fprintf(stderr, "%s [FLAGS] %s [COMMAND FLAGS]\n", basename(name), command->name);
  optly__usage_flags(command->flags);
}

OPTLYDEF void optly_usage(char const *const bin_path, OptlyCommand *commands, OptlyFlag *flags) {
  // NOTE: Copying const path into non-const path bause of libgen basename
  char name[strlen(bin_path) + 1];
  strcpy(name, bin_path);
  fprintf(stderr, "%s [FLAGS] <COMMAND> [COMMAND FLAGS]\n", basename(name));
  optly__usage_commands_list(commands);
  optly__usage_flags(&flags);
}

/**
 * Check if argument matches a flag definition.
 */
static OPTLYDEF bool optly__flag_matches(const char *arg, const OptlyFlag *flag) {
    bool is_short = arg[1] != '-';

    return (!is_short && strcmp(arg + 2, flag->fullname) == 0) ||
           (is_short && (arg[1] == flag->shortname));

}

/**
 * Find a flag by argument.
 */
static OPTLYDEF OptlyFlag *optly__find_flag(const char *arg, OptlyFlag *flags) {
    for (OptlyFlag *f = flags; !optly_is_flag_null(f); f++) {
        if (optly__flag_matches(arg, f)) {
            return f;
        }
    }

    return NULL;
}

static OPTLYDEF bool optly__flag_set_value(OptlyFlag *flag, char *value) {
    flag->value.as_int64 = 0;

    switch (flag->type) {
        case OPTLY_TYPE_CHAR:   flag->value.as_char   = *value; break;
        case OPTLY_TYPE_STRING: flag->value.as_string = value; break;
        case OPTLY_TYPE_INT8:   flag->value.as_int8   = atoll(value); break;
        case OPTLY_TYPE_INT16:  flag->value.as_int16  = atoll(value); break;
        case OPTLY_TYPE_INT32:  flag->value.as_int32  = atoll(value); break;
        case OPTLY_TYPE_INT64:  flag->value.as_int64  = atoll(value); break;
        case OPTLY_TYPE_UINT8:  flag->value.as_uint8  = atoll(value); break;
        case OPTLY_TYPE_UINT16: flag->value.as_uint16 = atoll(value); break;
        case OPTLY_TYPE_UINT32: flag->value.as_uint32 = atoll(value); break;
        case OPTLY_TYPE_UINT64: flag->value.as_uint64 = atoll(value); break;
        case OPTLY_TYPE_FLOAT:  flag->value.as_float  = atof(value); break;
        case OPTLY_TYPE_DOUBLE: flag->value.as_double = atof(value); break;
        /* case OPTLY_TYPE_PTR:    flag->value.as_ptr    = value; break; */

        case OPTLY_TYPE_BOOL: {
            if (!value) {
                flag->value.as_bool = true;
                break;
            }

            char lower[8];
            size_t len = strlen(value);

            if (len >= sizeof(lower)) {
                len = sizeof(lower) - 1;
            }

            for (size_t i = 0; i < len; i++) {
                lower[i] = (char)tolower((unsigned char)value[i]);
            }

            lower[len] = '\0';

            if (strcmp(lower, "true") == 0 || strcmp(lower, "t") == 0 ||
                strcmp(lower, "yes") == 0 || strcmp(lower, "y") == 0) {
                flag->value.as_bool = true;
            } else if (strcmp(lower, "false") == 0 || strcmp(lower, "f") == 0 ||
                strcmp(lower, "no") == 0 || strcmp(lower, "n") == 0) {
                flag->value.as_bool = false;
            } else {
                flag->value.as_int8 = -1;
            }

            break;
        }
    }

    if (flag->type > OPTLY_TYPE_INT8 && flag->type < OPTLY_TYPE_DOUBLE && flag->value.as_int64 == 0) {
        fprintf(stderr, "ERROR: Argument '%s' is not a number\n", flag->fullname);
        return false;
    }

    if (flag->type == OPTLY_TYPE_BOOL && flag->value.as_int8 == -1) {
        fprintf(stderr, "Invalid boolean value for flag '%s': %s\n",
                flag->fullname, value);
        return false;
    }

    return true;
}

/**
 * Parse flags from argv.
 */
static OPTLYDEF void optly__parse_flags(char ***argv_ptr, int *argc_ptr, OptlyFlag *flags) {
    char **argv = *argv_ptr;
    int    argc = *argc_ptr;

    if (!argv || argc <= 0 || !*argv) {
        return;
    }

    char *arg = *argv;

    // Detect batch short flags like `-abc`
    bool is_batch_short = (arg[0] == '-' && arg[1] != '-' && strlen(arg) > 2) && arg[2] != '=';

    if (is_batch_short) {
        if (strchr(arg, '=') != NULL) {
            *argv_ptr = argv;
            *argc_ptr = argc;
            return;
        }

        for (char *c = &arg[1]; *c; c++) {
            char sarg[3];
            snprintf(sarg, sizeof(sarg), "-%c", *c);

            OptlyFlag *flag = optly__find_flag(sarg, flags);

            if (!flag) {
                fprintf(stderr, "Unknown short flag: %s\n", sarg);
                continue;
            }

            if (flag->type != OPTLY_TYPE_BOOL) {
                fprintf(stderr, "Error: cannot batch non-boolean flags (invalid flag in %s)\n", arg);
                continue;
            }

            flag->value.as_bool = true;
        }

        *argv_ptr = argv;
        *argc_ptr = argc;
        return;
    }

    // Handle single flag: long or short
    char *value = NULL;

    char *eq = strchr(arg, '=');
    char  tmp[256];

    if (eq) {
        size_t len = strlen(arg);
        if (len >= sizeof(tmp)) len = sizeof(tmp) - 1;
        memcpy(tmp, arg, len);
        tmp[len] = '\0';

        char *eq2 = strchr(tmp, '=');
        *eq2 = '\0';
        arg   = tmp;
        value = eq + 1;
    }

    OptlyFlag *flag = optly__find_flag(arg, flags);

    if (!flag) {
        fprintf(stderr, "Unknown flag: %s\n", arg);
        return;
    }

    // If no value provided inline and next arg looks like a value (not a flag)
    if (!value && argc > 1 && argv[1][0] != '-') {
        value = argv[1];
        SHIFT_ARG(argv, argc);
    }

    // Handle boolean flags with implied true or explicit values
    if (flag->type == OPTLY_TYPE_BOOL) {
        if (!value) {
            flag->value.as_bool = true;
        } else {
            if (strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") == 0 || strcasecmp(value, "y") == 0 || strcmp(value, "1") == 0) {
                flag->value.as_bool = true;
            } else if (strcasecmp(value, "false") == 0 || strcasecmp(value, "no") == 0 || strcasecmp(value, "n") == 0 || strcmp(value, "0") == 0) {
                flag->value.as_bool = false;
            } else {
                fprintf(stderr, "Invalid boolean value for flag %s: %s\n", arg, value);
            }
        }
    } else {
        if (!value) {
            fprintf(stderr, "Missing value for flag: %s\n", arg);
        } else if (!optly__flag_set_value(flag, value)) {
            fprintf(stderr, "Failed to set value for flag: %s\n", arg);
        }
    }

    *argv_ptr = argv;
    *argc_ptr = argc;
}

/**
 * Parse a command from argv.
 */
static OPTLYDEF void optly__parse_command(const char *arg, OptlyArgs *args, OptlyCommand *commands) {
    for (OptlyCommand *cmd = commands; !optly_is_command_null(cmd); cmd++) {
        if (strcmp(arg, cmd->name) == 0) {
            args->command = cmd;
            return;
        }
    }
}

/**
 * Parse OPTLY arguments.
 */
OPTLYDEF OptlyArgs optly_parse_args(int argc, char *argv[], OptlyFlag *flags, OptlyCommand *commands) {
    assert(argc > 0);

    OptlyArgs args = {0};
    strcpy(args.bin_path, argv[0]);
    SHIFT_ARG(argv, argc);

    while (argc > 0) {
        char *arg = *argv;

        if (!arg) {
            break;
        }


        if (arg[0] == '-') {
            if (args.command) {
                optly__parse_flags(&argv, &argc, *args.command->flags);
            } else {
                optly__parse_flags(&argv, &argc, flags);
            }
        } else {
            optly__parse_command(arg, &args, commands);
        }

        SHIFT_ARG(argv, argc);
    }

    return args;
}

#endif // OPTLYARGS_IMPLEMENTATION

// TODO: Add sub-commands
// TODO: Add positional arguments
// TODO: Add marker for required flag
// TODO: Add ability to ignore unknown flags
// TODO: Add descriptions for usage
// TODO: Come up with a way to get a flag by its name
