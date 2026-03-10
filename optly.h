/*
    optly.h — v1.1.0
    Single-header command line argument parser for C.

    Features
    --------
    * STB-style single-header library
    * Commands and command-specific flags
    * Long flags (--verbose)
    * Short flags (-v)
    * Batched short flags (-abc)
    * Inline flag values (--threads=4)
    * Separate flag values (--threads 4)
    * Typed flag values
    * Positional arguments
    * Optional commands
    * Optional flags
    * No dynamic memory allocation
    * Portable C (C99)

    Basic Usage
    -----------

        #define OPTLYARGS_IMPLEMENTATION
        #include "optly.h"

        static OptlyFlag flags[] = {
            { "verbose", 'v', "Enable verbose output", {false}, OPTLY_TYPE_BOOL },
            { "threads", 't', "Worker threads", {4}, OPTLY_TYPE_UINT32 },
            NULL_FLAG
        };

        static OptlyCommand commands[] = {
            OPTLY_CMD("run",
                { "port", 'p', "Server port", {8080}, OPTLY_TYPE_UINT16 }
            ),
            NULL_COMMAND
        };

        int main(int argc, char **argv)
        {
            OptlyArgs args = optly_parse_args(argc, argv, flags, commands);

            if (args.command)
                printf("Command: %s\n", args.command->name);

            printf("Threads: %u\n", flags[1].value.as_uint32);
        }

    Commands
    --------

    Commands are positional tokens that do not begin with '-'.

        app build
        app run

    Each command may define its own flags.

    Flags
    -----

    Flags may be long or short.

        --verbose
        -v

    Flags may have values.

        --threads=4
        --threads 4
        -t 4

    Short flags can be batched.

        -abc  ->  -a -b -c

    Positional Arguments
    --------------------

    Any non-flag tokens after command selection are stored as positional arguments.

        app build file1 file2

    Access:

        args.positionals.values[i]

    Licese
    ------

    MIT/Public domain - choose whitchever you prefer
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
// same with positional arguments
#define OPTLY_MAX_POSITIONALS 64
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
} OptlyFlagType;

typedef union OtplyFlagValue {
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
    char *description;
    OptlyFlagValue value; ///< Value for the flag, based on flag type
    OptlyFlagType type; ///< Flag type
} OptlyFlag;

/**
 * Represents a OPTLY command with associated flags.
 */
typedef struct {
    char       *name;                       ///< Command name
    char *description;
    OptlyFlag  *flags;    ///< Array of flags termitaed by NULL_FALG macro
} OptlyCommand;

typedef struct {
    char *values[OPTLY_MAX_POSITIONALS];
    size_t count;
} OptlyPositionals;

typedef struct OptlyArgs {
    OptlyCommand *command;
    OptlyPositionals positionals;
    char bin_path[MAX_PATH_LENGTH];
} OptlyArgs;

#define NULL_FLAG    { .fullname = NULL, .shortname = 0, .value = { .as_int64 = 0 }, .type = 0 }
#define NULL_COMMAND { .name = NULL, .flags =  NULL  }

#define OPTLY_CMD(name, description, ...) { (name), (description), (OptlyFlag[]) {__VA_ARGS__ NULL_FLAG} }

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
    return cmd == NULL || cmd->name == NULL;
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
#include <stdlib.h>

#define SHIFT_ARG(argv, argc) (++(argv), --(argc))

static OPTLYDEF size_t optly__flag_print_width(OptlyFlag *flags) {
    size_t max = 0;

    for (OptlyFlag *flag = flags; !optly_is_flag_null(flag); flag++) {
        size_t len = 0;

        if (flag->shortname) {
            len += 2;
        }

        if (flag->fullname) {
            len += strlen(flag->fullname) + 3;
        }

        if (len > max) {
            max = len;
        }
    }

    return max;
}

static OPTLYDEF size_t optly__command_print_width(OptlyCommand *commands) {
    size_t max = 0;

    for (OptlyCommand *cmd = commands; !optly_is_command_null(cmd); cmd++) {
        size_t len = strlen(cmd->name);

        if (len > max) {
            max = len;
        }
    }

    return max;
}

/**
 * Print list of available commands.
 */
static OPTLYDEF void optly__usage_commands_list(OptlyCommand *commands) {
    fprintf(stderr, "COMMANDS\n");
    size_t pad = optly__command_print_width(commands);

    for (OptlyCommand *cmd = commands; !optly_is_command_null(cmd); cmd++) {
        fprintf(stderr, "  %-*s  %s\n", (int)pad, cmd->name, cmd->description);
    }
}

/**
 * Print list of flags.
 */
static OPTLYDEF void optly__usage_flags(OptlyFlag *flags) {
    if (!flags) {
        return;
    }

    fprintf(stderr, "FLAGS\n");
    size_t pad = optly__flag_print_width(flags);

    for (OptlyFlag *flag = flags; !optly_is_flag_null(flag); flag++) {
        size_t buflen = 128;
        char buf[buflen];

        snprintf(buf, buflen, "  ");

        if (flag->shortname && flag->fullname) {
            snprintf(buf, buflen, "-%c --%s", flag->shortname, flag->fullname);
        } else if (flag->fullname) {
            snprintf(buf, buflen, "--%s", flag->fullname);
        }
        else {
            snprintf(buf, buflen, "-%c", flag->shortname);
        }

        // NOTE: @print_flag_type
        fprintf(stderr, "  %-*s  %s\n", (int)pad, buf, flag->description);

    }
}

OPTLYDEF void optly_command_usage(char const *const bin_path, OptlyCommand *command) {
  const char *name = strrchr(bin_path, '/');
  name = name ? name + 1 : bin_path;
  fprintf(stderr, "%s [FLAGS] %s [COMMAND FLAGS]\n", name, command->name);
  optly__usage_flags(command->flags);
}

OPTLYDEF void optly_usage(char const *const bin_path, OptlyCommand *commands, OptlyFlag *flags) {
  const char *name = strrchr(bin_path, '/');
  name = name ? name + 1 : bin_path;
  fprintf(stderr, "%s [FLAGS] <COMMAND> [COMMAND FLAGS]\n", name);
  optly__usage_commands_list(commands);
  optly__usage_flags(flags);
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
    char *end = "";

    switch (flag->type) {
        case OPTLY_TYPE_CHAR:   flag->value.as_char   = *value; break;
        case OPTLY_TYPE_STRING: flag->value.as_string = value; break;
        case OPTLY_TYPE_INT8:   flag->value.as_int8   = strtoll(value, &end, 10); break;
        case OPTLY_TYPE_INT16:  flag->value.as_int16  = strtoll(value, &end, 10); break;
        case OPTLY_TYPE_INT32:  flag->value.as_int32  = strtoll(value, &end, 10); break;
        case OPTLY_TYPE_INT64:  flag->value.as_int64  = strtoll(value, &end, 10); break;
        case OPTLY_TYPE_UINT8:  flag->value.as_uint8  = strtoll(value, &end, 10); break;
        case OPTLY_TYPE_UINT16: flag->value.as_uint16 = strtoll(value, &end, 10); break;
        case OPTLY_TYPE_UINT32: flag->value.as_uint32 = strtoll(value, &end, 10); break;
        case OPTLY_TYPE_UINT64: flag->value.as_uint64 = strtoll(value, &end, 10); break;
        case OPTLY_TYPE_FLOAT:  flag->value.as_float  = strtod(value, &end); break;
        case OPTLY_TYPE_DOUBLE: flag->value.as_double = strtod(value, &end); break;

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

    if (*end != '\0') {
        fprintf(stderr, "ERROR: Argument '%s' is not a number (%s)\n", flag->fullname, value);
        return false;
    }

    if (flag->type == OPTLY_TYPE_BOOL && flag->value.as_int8 == -1) {
        fprintf(stderr, "Invalid boolean value for flag '%s': %s\n", flag->fullname, value);
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

static OPTLYDEF void optly__push_positionals(OptlyArgs *args, char *value) {
    assert(args->positionals.count < OPTLY_MAX_POSITIONALS);
    args->positionals.values[args->positionals.count++] = value;
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
                optly__parse_flags(&argv, &argc, args.command->flags);
            } else if (flags) {
                optly__parse_flags(&argv, &argc, flags);
            }
            else {
                optly__push_positionals(&args, arg);
            }
        } else {
            if (commands && !args.command) {
                optly__parse_command(arg, &args, commands);
            }

            if (!args.command) {
                optly__push_positionals(&args, arg);
            }
        }

        SHIFT_ARG(argv, argc);
    }

    return args;
}

#endif // OPTLYARGS_IMPLEMENTATION

// TODO: Add sub-commands
// TODO: Add ability to ignore unknown flags
// TODO: Come up with a way to get a flag by its name
// TODO: Add ability to disable error messages @disable_warnings (logcie?)
// TODO: Should I print type of flag value? @print_flag_type

/*
   ------------------------------------------------------------------------------
   This software is available under 2 licenses -- choose whichever you prefer.
   ------------------------------------------------------------------------------
   ALTERNATIVE A - MIT License
   Copyright (c) 2026 Nikita Chukov nikita_chul@mail.ru
   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   ------------------------------------------------------------------------------
   ALTERNATIVE B - Public Domain (www.unlicense.org)
   This is free and unencumbered software released into the public domain.
   Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
   software, either in source code form or as a compiled binary, for any purpose,
   commercial or non-commercial, and by any means.
   In jurisdictions that recognize copyright laws, the author or authors of this
   software dedicate any and all copyright interest in the software to the public
   domain. We make this dedication for the benefit of the public at large and to
   the detriment of our heirs and successors. We intend this dedication to be an
   overt act of relinquishment in perpetuity of all present and future rights to
   this software under copyright law.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   ------------------------------------------------------------------------------
*/
