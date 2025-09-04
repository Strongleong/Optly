# Optly

My own single file header-only library for handling parsing CLI arguments

It supports parsing flags with different "styles":
 - `./app --flag --another`
 - `./app -f -a`
 - `./app -fa`
 - `./app --value 123`
 - `./app --value=123`
 - `./app -v 123`
 - `./app -v=123`

Note that mixing "batch" args and args with values like this (`./app -fav=123`) is not supported and can lead to unexpected results

Also this Optly supports commands and its flags:

 - `./app list`
 - `./app download -u example.com`

Subcommands (like `./app list help`) and positional arguments are not implemented yet

## How to use

Here is basic example ([./examples/simple.c](./examples/simple.c)):

```c
#define OPTLYARGS_IMPLEMENTATION
#include <optly.h>

#include <stdio.h>

static OptlyFlag flags[] = {
  { "value",  'v', {0},     OPTLY_TYPE_UINT32 },
  { "switch", 's', {false}, OPTLY_TYPE_BOOL },
  NULL_FLAG,
};

static OptlyFlag download_flags[] = {
  { "url",    'u', {NULL},  OPTLY_TYPE_STRING },
  { "switch", 's', {false}, OPTLY_TYPE_BOOL },
  NULL_FLAG,
};

static OptlyCommand commands[] = {
  { "help",     { NULL } },
  { "download", {download_flags} },
  NULL_COMMAND,
};

int main(int argc, char *argv[]) {
  OptlyArgs args = optly_parse_args(argc, argv, flags, commands);

  if (flags[2].value.as_bool || args.command == &commands[0]) {
    optly_usage(args.bin_path, commands, flags);
    return 0;
  }

  printf("Binary: %s\n", args.bin_path);

  if (args.command) {
    if (args.command == &commands[1]) {
      optly_usage(args.bin_path, commands, flags);
    }

    printf("Command: %s\n", args.command->name);
  }

  printf("Value  = %u\n", flags[0].value.as_uint32);
  printf("Switch = %s\n\n", flags[1].value.as_bool ? "true" : "false");

  printf("Download url    = %s\n", download_flags[0].value.as_string);
  printf("Download switch = %s\n", download_flags[1].value.as_bool ? "true" : "false");

  return 0;
}
```

And now goes in-depth yapping.

As you see, the main function of this library is `optly_parse_args()`.
It takes `argc`, `argv` and arrays of `OptlyCommand`s and `OptlyFlag`s.
I will call arrays of `OptlyCommand`s and `OptlyFlag`s that you pass to `optly_parse_args()` just (your app’s) commands and flags respectively.

---

### Flags

`OptlyFlag` structure has (in order): long name (`--value`), short name (`-v`), value and type.
If you set value of `OptlyFlag` in flags array it will be its default value.
If you set `NULL` instead, it will mark that this flag is required and optly will issue an error if that flag was not provided at runtime.

Flags arrays are always **NULL-terminated** using `NULL_FLAG`.

#### Supported types

- `OPTLY_TYPE_BOOL` → for switches. Will be `true` if flag is present, or whatever default you give. You can also explicitly pass values like `yes/no`, `y/n`, `true/false`, `t/f`, `1/0`.
- `OPTLY_TYPE_CHAR` → stores a single character. Takes first character of the provided string.
- `OPTLY_TYPE_STRING` → takes a string argument as-is.
- `OPTLY_TYPE_INT8` / `OPTLY_TYPE_INT16` / `OPTLY_TYPE_INT32` / `OPTLY_TYPE_INT64` → signed integer types of different widths.
- `OPTLY_TYPE_UINT8` / `OPTLY_TYPE_UINT16` / `OPTLY_TYPE_UINT32` / `OPTLY_TYPE_UINT64` → unsigned integer types.
- `OPTLY_TYPE_FLOAT` → single-precision floating point.
- `OPTLY_TYPE_DOUBLE` → double-precision floating point.

All flag types (except implicit booleans) accept arguments in the form `--flag=value` or `--flag value`.
If conversion fails (e.g. passing `--int=hello`), optly prints an error.

Flags with type `OPTLY_TYPE_BOOL` are special: they don’t require a value. Writing just `--verbose` is enough to flip them on. But if you really want, you can still do `--verbose=false` or `-v no`.

---

### Commands

`OptlyCommand` structure is even simpler: it has a name (`"download"`, `"help"`, etc.) and its own **NULL-terminated** array of flags (not `NULL_FLAG`-termntaed. Just `NULL`).
Each entry in your commands array represents a single subcommand your app supports.
The parser will compare the next arg against this list of names, and if there’s a match, it switches into “command mode” — meaning all following flags are matched against that command’s flag array, not the global one.

Commands arrays are always **NULL-terminated** using `NULL_COMMAND`.

If a command doesn’t take any flags, just pass `{ NULL }` as its flag array.

---

### How it behaves

When parser encounters a command, all remaining args are considered to belong to that command.
So for example:

- `./app --url example.com` → will issue a warning, because top-level flags don’t have `url`.
- `./app download --url example.com` → works fine, because `download` command has its own `url` flag.
- `./app --switch download` → sets the top-level `switch` only, because `download` here is just another positional argument.
- `./app download --switch` → sets the `switch` inside `download_flags`, leaving the top-level one untouched.

In other words, each command comes with its own separate set of flags, and optly will switch context as soon as it sees the command name.
Subcommands like `./app list help` or positional arguments (`./app input.txt`) are not there yet. Maybe one day, but not today.

Example output of `optly_usage()`:

```console
simple [FLAGS] <COMMAND> [COMMAND FLAGS]
COMMANDS
  help
  download
FLAGS
  -v --value
  -s --switch
  -s --help
```

Example output of `optly_command_usage()`:

```console
simple [FLAGS] download [COMMAND FLAGS]
FLAGS
  -u --url
  -s --switch
```

### Usage functions

Optly also ships with a couple of helper functions to quickly print usage info:

 - `optly_usage(bin_path, commands, flags)` Prints a top-level usage message. Shows available global flags and commands.
 - `optly_command_usage(bin_path, command)` Prints usage message for a single command, including its flags.

They both use the same arrays you already pass into the parser, so you don’t need to maintain a separate “usage string” by hand.
Good for `--help` flags, or when you want to yell at the user for giving you wrong args.

## How to run tests

```console
git clone https://github.com/strongleong/optly
cd optly
./build.sh && ./out/tessts
```

## `./build.sh`

I use plain bash script `./build.sh` as "build system" with couple flags that may be useful sometimes (all flags that `./build.sh` supports I actually used)

Build flags:

```console
-d --debug     Compile with debug flags
-p --profile   Compile with profile flags
-s --silent    Compile without unnececary output
-o --outdir    Set output dir (default: ./out)
-c --compiler  Set which compier to use (default: clang)
-h --help      Print help
```

