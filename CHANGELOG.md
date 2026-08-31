# Changelog

## Upcoming

### Added

- `OPTLY_NULL_FLAG`, `OPTLY_NULL_COMMAND` and `OPTLY_NULL_POSITIONAL`.
  The old spellings still work, but they are deprecated and will be
  removed in v3.
- `optly_flag_value_enum()` is declared in the header. It has always been
  there, but only code that defined `OPTLY_IMPLEMENTATION` could reach it.
- A test suite in the .tspec format, run with strum. 43 tests across 11
  directories, covering flag forms, every value type, commands and
  subcommands, positionals, error accumulation, enums, usage output and the
  exit behaviour.

### Fixed

- **A crash reachable from any optly program's command line.** A flag written
  as `--name=value` where the name is longer than `OPTLY_FLAG_BUFFER_LENGTH`
  (256) was truncated before its `=`, and the split then wrote through a NULL
  pointer. Such a flag is now reported as unknown.
- `OPTLY_HELP_SHORT_FLAG` and `OPTLY_VERSION_SHORT_FLAG` were honoured when
  parsing but ignored when printing usage, so overriding them left a program
  advertising flags it did not accept.
- `optly_get_positional()` was declared twice in the header.

### Changed

- `build.sh` is replaced by `build.c`. The script never compiled the examples:
  it guarded the compile with a variable it never assigned. Its flag handling
  was broken too, and bash ruled out Windows. See the README for usage.

## v2.3.6

### Fixed

- Optly no longer includes `<strings.h>`. It is POSIX-only and MSVC does not
  ship it, so the header failed to compile on Windows. Nothing in optly used
  it.
- The `#warning` about an old Logcie version is only emitted on GCC and Clang.
  `#warning` is not standard C before C23, so on other compilers the fallback
  path warned about itself.
- Internal function names no longer contain a double underscore, which is
  reserved by C and C++.
- The usage example in the header printed `address->values[1]`. The first
  positional value is at index 0.
- Assorted typos in the header docs.

### Changed

- Readme is updated, fixed a lot of typos.

### Known issues

- `optly_flag_value_string()` returns char *, but the pointer is only writable
  when the flag was actually present - a missing flag yields a string literal.
  The return type becomes const char * in v3.
