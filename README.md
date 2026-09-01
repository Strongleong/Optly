# Optly

Optly is a **single-header command line argument parser for C (C99)**.

It is designed to be **small, dependency‑free, and allocation‑free**,
making it ideal for small CLI tools and embedded-style programs.

[![.github/workflows/main.yml](https://github.com/Strongleong/Optly/actions/workflows/main.yml/badge.svg)](https://github.com/Strongleong/Optly/actions/workflows/main.yml)

## Features

-   Single-header library
-   No dynamic memory allocation
-   Portable C99
-   Commands and nested subcommands
-   Command-specific flags
-   Long flags (`--verbose`)
-   Short flags (`-v`)
-   Batched short flags (`-abc`)
-   Inline flag values (`--threads=4`)
-   Separate flag values (`--threads 4`)
-   Typed flag values
-   Positional arguments
-   Optional and required flags
-   Optional automatic generation and handling of `--help/-h` and `--version/-v` flags
-   Optional automatic generation and handling of `help` / `help cmd` and `version` commands

## Installation

Just drop `optly.h` into your project.

In **one C file**:

``` c
#define OPTLY_IMPLEMENTATION
#include "optly.h"
```

In other files:

``` c
#include "optly.h"
```

## Example

``` c
#define OPTLY_IMPLEMENTATION
#include "optly.h"
#include <stdio.h>

int main(int argc, char **argv)
{
  OptlyCommand cmd = {
    .name = "app",

    .flags = optly_flags(
      optly_flag_bool("verbose", 'v', "Enable verbose output", .value.as_bool = false),
      optly_flag_uint32("threads", 't', "Worker threads", .value.as_uint32 = 4)
    ),

    .commands = optly_commands(
      optly_command("run", "Run server",
        .flags = optly_flags(
          optly_flag_uint16("port", 'p', "Server port", .value.as_uint16 = 8080)
        )
      )
    )
  };

  optly_parse_args(argc, argv, &cmd);

  printf("threads: %u\n", optly_flag_value_uint32(&cmd, "threads"));

  if (cmd.next_command) {
    printf("command: %s\n", cmd.next_command->name);
    printf("port: %u\n", optly_flag_value_uint16(cmd.next_command, "port"));
  }
}
```

Run:

``` bash
./app --threads 8 run --port 9000
```

## Error handling

`optly_parse_args()` collects every problem it finds instead of stopping at the
first one, and returns them:

``` c
OptlyErrors errs = optly_parse_args(argc, argv, &cmd);

for (size_t i = 0; i < optly_errors_count(&errs); i++) {
  OptlyError e = optly_errors_at(&errs, i);
  printf("%s%s%s\n", optly_error_message(e.kind), e.arg ? ": " : "", e.arg ? e.arg : "");
}
```

`optly_error_print(&errs)` does the same thing if the default wording is fine.

**By default optly calls `exit()` when parsing fails.** A CLI parser runs once,
at startup, and bad arguments mean the program should not continue. To take
that decision yourself -- in tests, or when you want to print your own message
-- define:

``` c
#define OPTLY_NO_EXIT
```

Optly also logs each error as it happens. Out of the box that goes to `stderr`
via `fprintf`. If [logcie](https://github.com/Strongleong/logcie) is included
before optly, its module logging is used instead, so your application decides
where optly's output goes. To route it somewhere else entirely, define
`OPTLY_LOG` before including optly:

``` c
#define OPTLY_LOG(level, ...) my_logger(#level, __VA_ARGS__)
```

## Flags

Supported forms:

    --verbose
    -v
    --threads 4
    --threads=4
    -t 4

Short flags can be batched:

    -abc

Equivalent to:

    -a -b -c

The last flag in a batch may take a value, the way tar does it:

    tar -xzvf archive.tar

Everything before the last one must be boolean -- a flag in the middle has no
way to say where its value ends.

## Commands

Commands are positional tokens:

    app run
    app build
    app run check

Each command may define its own:

-   flags
-   subcommands
-   positional arguments

## Positional Arguments

Declare them on the command that accepts them:

``` c
.positionals = optly_positionals(
  optly_positional("files", "Files to build", .min = 1, .max = 0)
)
```

`min` is how many are required, `max` how many are allowed; `max = 0` means any
number. Everything after a bare `--` is treated as positional, even if it looks
like a flag:

    app build file1 file2
    app build -- --not-a-flag

Access them via:

``` c
OptlyPositional *p = optly_get_positional(&cmd, "files");

for (size_t i = 0; i < p->count; i++) {
  printf("%s\n", p->values[i]);
}
```

## Usage Helpers

``` c
optly_usage(&cmd);                 // usage for the whole program
optly_usage(cmd.next_command);     // usage for the selected command
```


## Help and version command/flag generation

You can define

```c
  #define OPTLY_GEN_HELP_FLAG
  #define OPTLY_GEN_HELP_COMMAND
```

to generate help flag `--help | -h` and/or help command `help cmd`, or

```c
#define OPTLY_GEN_VERSION_FLAG
#define OPTLY_GEN_VERSION_COMMAND
```

to generate version flag `--version | -v` and/or version command `version`.

If help/version command/flag would be found during parsing usage would be
automatically called and `exit(0)` is called.

Note that user defined flags with `-h`/`-v` would interfere with generated flags.
Their short forms can be moved with `OPTLY_HELP_SHORT_FLAG` and
`OPTLY_VERSION_SHORT_FLAG`.

`OPTLY_GEN_VERSION_FLAG` and `OPTLY_GEN_VERSION_COMMAND` add a fourth parameter
to `optly_parse_args()`, the version string:

``` c
optly_parse_args(argc, argv, &cmd, "1.0.0");
```

## Configuration

Define these before including optly:

| Macro                      | Default             | Effect                                           |
| -----                      | -------             | ------                                           |
| `OPTLY_NO_EXIT`            | off                 | Never call `exit()`; return the errors instead   |
| `OPTLY_MAX_POSITIONALS`    | 64                  | Values one positional can hold                   |
| `OPTLY_MAX_ERRORS`         | 32                  | Errors collected before further ones are dropped |
| `OPTLY_FLAG_BUFFER_LENGTH` | 256                 | Buffer used while formatting help output         |
| `OPTLY_LOG`                | `fprintf` to stderr | Where optly's own messages go                    |
| `OPTLY_HELP_SHORT_FLAG`    | `"-h"`              | Short flag for generated help                    |
| `OPTLY_VERSION_SHORT_FLAG` | `"-v"`              | Short flag for generated version                 |
| `OPTLYDEF`                 | empty               | Linkage of the public functions                  |

## C only

Optly is a C library. It is not tested as C++ and does not try to compile as
C++ -- use argparse, CLI11 or cxxopts there.

## Building

Optly itself is a single header -- there is nothing to build to use it. The
repository ships `build.c`, which compiles the examples and runs the tests.
Compile it once, then use the binary:

``` bash
cc build.c -o build

./build          # compile every example into ./out
./build tests    # run the .tspec suite
./build clean    # empty the output directory
```

Run `./build help` for more info.

## Tests

Tests live in `./tests/`, one directory per concept. Each holds a small C
fixture and a `test.tspec` that compiles it, runs it, and compares the exact
bytes it writes.

They are written in the .tspec format and run with strum:

``` bash
./build tests
```

strum has to be on your `PATH`. Use `--tspec-runner` to point at a different
implementation of the format.

Read more about tspec and strum at [https://github.com/strongleong/strum](https://github.com/strongleong/strum)

## Design Goals

Optly focuses on:

-   **minimal runtime overhead**
-   **zero allocations**
-   **simple static configuration**

This makes it suitable for:

-   CLI utilities
-   embedded tools
-   static binaries
-   low-level C projects

## License

Optly is dual licensed:

-   MIT License
-   Public Domain (Unlicense)

Choose whichever works best for your project.
