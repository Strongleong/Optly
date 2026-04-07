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

static OptlyCommand cmd = {
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

int main(int argc, char **argv)
{
  optly_parse_args(argc, argv, &cmd);

  printf("threads: %u\n", optly_uint32(&cmd, "threads"));

  if (cmd.next_command) {
    printf("command: %s\n", cmd.next_command->name);
    printf("port: %u\n", optly_uint16(cmd.next_command, "port"));
  }
}
```

Run:

``` bash
./app --threads 8 run --port 9000
```

## Error handling

This library reports its errors by itself, no complicated error handling required.

By default reporting is disabled, but Optly provides [logcie](https://github.com/Strongleong/logcie/tree/custom-format?tab=readme-ov-file#usage-in-libraries) interface.

Just put logcie somewhre next to optly in your project, or define logging macros

```c
#define OPTLY_LOG_BACKEND(level, ...) fprintf(stderr, ##level ": " __VA_ARGS__);
```

You can disable or enable certan logging levels in compiile time:

```c
#define OPTLY_LOG_DEBUG // define to nothing
```

To control logging in runtime check out [logcie](https://github.com/strongleong/logcie).

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

*(Batched flags must be boolean.)*

## Commands

Commands are positional tokens:

    app run
    app build
    app run check

Each command may define its own:

-   flags
-   subcommands
-   positional arguments

# Positional Arguments

Example:

    app build file1 file2

Access them via:

``` c
OptlyPositional *p = optly_get_positional(cmd, "files");
```

## Usage Helpers

Print global usage:

``` c
optly_usage(argv[0], cmd.commands, cmd.flags);
```

Print command usage:

``` c
optly_command_usage(argv[0], command);
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
