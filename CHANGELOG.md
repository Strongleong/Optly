# Changelog

## Upcoming

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
