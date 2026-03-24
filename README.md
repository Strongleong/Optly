# Optly

Optly is a **single-header, allocation-free command-line argument parser for C99**.

It supports nested commands, typed flags, positional arguments, generated help, and
error aggregation while staying small and dependency-free.

---

## Installation

Copy `optly.h` into your project.

In **exactly one** `.c` file:

```c
#define OPTLY_IMPLEMENTATION
#include "optly.h"
```

In all other files:

```c
#include "optly.h"
```

---

## Quick start

```c
#define OPTLY_IMPLEMENTATION
#include "optly.h"
#include <stdio.h>

static OptlyCommand app = optly_command(
  "app",
  "Minimal example",
  .flags = optly_flags(
    optly_flag_bool("verbose", .shortname = 'v'),
    optly_flag_uint32("threads", .shortname = 't', .value.as_uint32 = 4)
  ),
  .commands = optly_commands(
    optly_command(
      "run",
      .flags = optly_flags(optly_flag_uint16("port", .shortname = 'p', .value.as_uint16 = 8080))
    )
  )
);

int main(int argc, char **argv) {
  OptlyErrors errs = optly_parse_args(argc, argv, &app);
  if (optly_errors_count(&errs) > 0) return 1;

  printf("threads=%u\n", optly_flag_value_uint32(&app, "threads"));

  if (optly_is_command(app.next_command, "run")) {
    printf("port=%u\n", optly_flag_value_uint16(app.next_command, "port"));
  }
}
```

---

## What Optly supports (full behavior reference)

### 1) Command tree behavior

- Commands are configured statically via `.commands = optly_commands(...)`.
- Matching is positional and left-to-right (`app deploy rollback`).
- Each command can have its own flags, subcommands, and positionals.
- Selected commands are linked through `next_command` so you can walk the parsed path.
- Unknown tokens are treated as positionals when a command defines positionals; otherwise they are unknown command errors.

### 2) Flag syntax behavior

Supported forms:

- Long flag as boolean switch: `--verbose`
- Short flag as boolean switch: `-v`
- Long flag with separate value: `--threads 8`
- Long flag with inline value: `--threads=8`
- Short flag with separate value: `-t 8`
- Short flag with inline value: `-t=8`
- Batched short booleans: `-abc` (equivalent to `-a -b -c`)

Rules:

- Batched flags must all be boolean.
- For non-boolean flags, the **last provided value wins**.
- Parsing stops flag interpretation after `--` and remaining tokens become positionals.

### 3) Typed flag behavior

Built-in typed helpers:

- `optly_flag_bool`
- `optly_flag_char`
- `optly_flag_string`
- `optly_flag_int8`, `optly_flag_int16`, `optly_flag_int32`, `optly_flag_int64`
- `optly_flag_uint8`, `optly_flag_uint16`, `optly_flag_uint32`, `optly_flag_uint64`
- `optly_flag_float`, `optly_flag_double`
- `optly_flag_enum`

Type conversion notes:

- Numeric conversion is decimal (`strtoll`, `strtoull`, `strtof`, `strtod`).
- Invalid numeric/float input produces `OPTLY_ERR_INVALID_VALUE`.
- Bool flags do not consume values; presence sets them to `true`.
- Enum flags store selected value at `as_enum[0]`, with allowed values in `as_enum[1..]`.

### 4) Required/default behavior

- Every flag can have a default value in `.value.<type_member>`.
- Mark required flags with `.required = true`.
- Missing required flags produce `OPTLY_ERR_MISSING_REQUIRED`.
- `present` is toggled true when a flag was seen in argv.

### 5) Positional argument behavior

Positionals are configured per command:

```c
.positionals = optly_positionals(
  optly_positional("input", .min = 1, .max = 1),
  optly_positional("extras", .min = 0, .max = 0)
)
```

Rules:

- `.min` / `.max` define accepted count for each positional.
- `.max = 0` means variadic/infinite.
- At most one variadic positional per command (duplicate variadics are an error).
- Too few values => `OPTLY_ERR_POSITIONAL_TOO_FEW`.
- Too many values => `OPTLY_ERR_POSITIONAL_TOO_MANY`.

### 6) Help behavior

Compile-time options:

- `#define OPTLY_GEN_HELP_FLAG` enables `-h`/`--help`.
- `#define OPTLY_GEN_HELP_COMMAND` enables `help <subcommand>`.

Behavior:

- Help prints usage for current command context.
- With help enabled and used, parser exits after printing help.

### 7) Error model behavior

`optly_parse_args` returns:

```c
typedef struct OptlyErrors {
  OptlyError items[OPTLY_MAX_ERRORS];
  size_t     count;
} OptlyErrors;
```

Use:

- `optly_errors_count(&errs)`
- `optly_errors_at(&errs, i)`
- `optly_error_message(kind)`

Error kinds:

- `OPTLY_ERR_UNKNOWN_FLAG`
- `OPTLY_ERR_UNKNOWN_COMMAND`
- `OPTLY_ERR_MISSING_VALUE`
- `OPTLY_ERR_INVALID_VALUE`
- `OPTLY_ERR_MISSING_REQUIRED`
- `OPTLY_ERR_POSITIONAL_TOO_FEW`
- `OPTLY_ERR_POSITIONAL_TOO_MANY`
- `OPTLY_ERR_DUPLICATE_VARIADIC`
- `OPTLY_ERR_BATCH_NON_BOOL`

By default, parse exits with failure when errors exist.

Define `OPTLY_NO_EXIT` to disable automatic exiting (recommended in tests and in applications that want custom error reporting).

### 8) Logging behavior

Optly logs via `OPTLY_LOG(...)`.

- If you already use Logcie (`LOGCIE` macros), Optly integrates automatically.
- Otherwise it falls back to `fprintf(stderr, ...)`.
- You can override `OPTLY_LOG` yourself before including `optly.h`.

### 9) Memory and limits behavior

- No dynamic allocations in Optly.
- Positionals are stored in fixed arrays on `OptlyPositional`.
- Tunable compile-time limits:
  - `OPTLY_MAX_POSITIONALS` (default 64)
  - `OPTLY_FLAG_BUFFER_LENGTH` (default 256)
  - `OPTLY_MAX_ERRORS` (default 32)

### 10) Accessor behavior

Lookup and accessor helpers:

- `optly_get_flag(flags, "name")`
- `optly_get_positional(command, "name")`
- `optly_is_command(command, "name")`
- Typed accessors: `optly_flag_value_uint32(...)`, etc.

If a flag is missing, typed accessors return zero-like defaults (`false`, `0`, `""`, or `NULL` for enum accessor).

---

## Examples in this repo

- `examples/full.c` — larger realistic CLI with nested commands.
- `examples/from_doc.c` — manually configured structs + helper macros side-by-side.
- `examples/enum_flag.c` — enum flags.
- `examples/positionals.c` — positional min/max distribution.
- `examples/error_handling.c` — collecting and printing parse errors.
- `examples/help_and_double_dash.c` — generated help and `--` delimiter behavior.

---

## Compile examples

```bash
cc -std=c99 -Wall -Wextra -I. examples/full.c -o /tmp/optly-full
```

Run tests:

```bash
cc -std=c99 -Wall -Wextra -pedantic -I. tests/test.c -o /tmp/optly-tests
/tmp/optly-tests
```

If your toolchain has strict C99 inline linkage behavior, use GNU inline mode:

```bash
cc -std=gnu99 -fgnu89-inline -Wall -Wextra -pedantic -I. tests/test.c -o /tmp/optly-tests
/tmp/optly-tests
```

---

## License

Dual licensed:

- MIT
- Unlicense (public domain)

Use whichever fits your project.
