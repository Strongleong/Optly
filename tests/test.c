#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define OPTLY_NO_EXIT
#define OPTLY_IMPLEMENTATION
#define OPTLY_LOG(...)
#include "optly.h"

/* -------------------------- tiny test framework -------------------------- */
#define ANSI_RED(x)   "\033[31m" x "\033[0m"
#define ANSI_GREEN(x) "\033[32m" x "\033[0m"

static int g_asserts = 0;
static int g_failed  = 0;

#define ASSERT_TRUE(expr)                                                                                 \
  do {                                                                                                    \
    g_asserts++;                                                                                          \
    if (!(expr)) {                                                                                        \
      fprintf(stderr, "    " ANSI_RED("ASSERT_TRUE failed") ": %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
      g_failed++;                                                                                         \
      return;                                                                                             \
    }                                                                                                     \
  } while (0)

#define ASSERT_FALSE(expr)                                                                                 \
  do {                                                                                                     \
    g_asserts++;                                                                                           \
    if ((expr)) {                                                                                          \
      fprintf(stderr, "    " ANSI_RED("ASSERT_FALSE failed") ": %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
      g_failed++;                                                                                          \
      return;                                                                                              \
    }                                                                                                      \
  } while (0)

#define ASSERT_EQ_INT(actual, expected)                                                                                            \
  do {                                                                                                                             \
    g_asserts++;                                                                                                                   \
    long long a__ = (long long)(actual);                                                                                           \
    long long e__ = (long long)(expected);                                                                                         \
    if (a__ != e__) {                                                                                                              \
      fprintf(stderr, "    " ANSI_RED("ASSERT_EQ_INT failed") ": got=%lld expected=%lld (%s:%d)\n", a__, e__, __FILE__, __LINE__); \
      g_failed++;                                                                                                                  \
      return;                                                                                                                      \
    }                                                                                                                              \
  } while (0)

#define ASSERT_EQ_STR(actual, expected)                                                                                                                      \
  do {                                                                                                                                                       \
    g_asserts++;                                                                                                                                             \
    const char *a__ = (actual);                                                                                                                              \
    const char *e__ = (expected);                                                                                                                            \
    if ((a__ == NULL && e__ != NULL) || (a__ != NULL && e__ == NULL) || (a__ && e__ && strcmp(a__, e__) != 0)) {                                             \
      fprintf(stderr, "    " ANSI_RED("ASSERT_EQ_STR failed") ": got=%s expected=%s (%s:%d)\n", a__ ? a__ : "NULL", e__ ? e__ : "NULL", __FILE__, __LINE__); \
      g_failed++;                                                                                                                                            \
      return;                                                                                                                                                \
    }                                                                                                                                                        \
  } while (0)

#define ASSERT_FLOAT_NEAR(actual, expected, eps)                                                                                                         \
  do {                                                                                                                                                   \
    g_asserts++;                                                                                                                                         \
    double a__ = (double)(actual);                                                                                                                       \
    double e__ = (double)(expected);                                                                                                                     \
    if (fabs(a__ - e__) > (eps)) {                                                                                                                       \
      fprintf(stderr, "    " ANSI_RED("ASSERT_FLOAT_NEAR failed") ": got=%f expected=%f eps=%f (%s:%d)\n", a__, e__, (double)(eps), __FILE__, __LINE__); \
      g_failed++;                                                                                                                                        \
      return;                                                                                                                                            \
    }                                                                                                                                                    \
  } while (0)

#define RUN_TEST(fn)                                      \
  do {                                                    \
    int before = g_failed;                                \
    fn();                                                 \
    if (g_failed == before)                               \
      fprintf(stderr, ANSI_GREEN("[PASS]") " %s\n", #fn); \
    else                                                  \
      fprintf(stderr, ANSI_RED("[FAIL]") " %s\n", #fn);   \
  } while (0)

/* ------------------------------ helpers ---------------------------------- */

static bool assert_err_count_failed = true;

static void do_assert_err_count(const OptlyErrors *errs, size_t expected) {
  assert_err_count_failed = true;
  ASSERT_EQ_INT((long long)optly_errors_count(errs), (long long)expected);
  assert_err_count_failed = false;
}

static void assert_err_count(const OptlyErrors *errs, size_t expected) {
  do_assert_err_count(errs, expected);

  if (!assert_err_count_failed) {
    return;
  }

  optly_error_print(errs);
}

static void assert_err_at(const OptlyErrors *errs, size_t idx, OptlyErrorKind kind, const char *arg) {
  OptlyError e = optly_errors_at(errs, idx);
  ASSERT_EQ_INT((int)e.kind, (int)kind);
  if (arg) ASSERT_EQ_STR(e.arg, arg);
}

#define ARGV(...) {__VA_ARGS__, NULL}

size_t count_argc(char *argv[]) {
  size_t i = 1;
  for (char **v = argv; *v; v++) ++i;
  return i;
}

/* ------------------------------ test cases -------------------------------- */

static void test_optional_flags_defaults(void) {
  OptlyCommand cmd = optly_command(
    "app",
    .flags = optly_flags(
      optly_flag_bool("verbose", .shortname = 'v', .value.as_bool = false),
      optly_flag_uint32("threads", .shortname = 't', .value.as_uint32 = 4)
    )
  );

  char       *argv[] = ARGV("app");
  OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
  assert_err_count(&errs, 0);
  ASSERT_FALSE(optly_flag_value_bool(&cmd, "verbose"));
  ASSERT_EQ_INT(optly_flag_value_uint32(&cmd, "threads"), 4);
}

static void test_long_short_and_batch_bools(void) {
  OptlyCommand cmd = optly_command(
    "app",
    .flags = optly_flags(
      optly_flag_bool("a", .shortname = 'a'),
      optly_flag_bool("b", .shortname = 'b'),
      optly_flag_bool("c", .shortname = 'c')
    )
  );

  char       *argv[] = ARGV("app", "-abc");
  OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
  assert_err_count(&errs, 0);
  ASSERT_TRUE(optly_flag_value_bool(&cmd, "a"));
  ASSERT_TRUE(optly_flag_value_bool(&cmd, "b"));
  ASSERT_TRUE(optly_flag_value_bool(&cmd, "c"));
}

static void test_inline_and_separate_values(void) {
  OptlyCommand cmd = optly_command(
    "app",
    .flags = optly_flags(
      optly_flag_uint32("threads", .shortname = 't'),
      optly_flag_string("name", .shortname = 'n')
    )
  );

  char       *argv[] = ARGV("app", "--threads=8", "--name", "Alice");
  OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
  assert_err_count(&errs, 0);
  ASSERT_EQ_INT(optly_flag_value_uint32(&cmd, "threads"), 8);
  ASSERT_EQ_STR(optly_flag_value_string(&cmd, "name"), "Alice");
}

static void test_short_value_equals_and_space(void) {
  OptlyCommand cmd = optly_command(
    "app",
    .flags = optly_flags(
      optly_flag_int64("value", .shortname = 'x')
    )
  );

  char       *argv[] = ARGV("app", "-x=77", "-x", "99");
  OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);

  // New value will override last one
  assert_err_count(&errs, 0);
  ASSERT_EQ_INT(optly_flag_value_int64(&cmd, "value"), 99);
}

static void test_typed_values(void) {
  OptlyCommand cmd = optly_command(
    "app",
    .flags = optly_flags(
      optly_flag_char("ch", .shortname = 'c'),
      optly_flag_int8("i8", .shortname = 'a'),
      optly_flag_int16("i16", .shortname = 'b'),
      optly_flag_int32("i32", .shortname = 'd'),
      optly_flag_int64("i64", .shortname = 'e'),
      optly_flag_uint8("u8", .shortname = 'f'),
      optly_flag_uint16("u16", .shortname = 'g'),
      optly_flag_uint32("u32", .shortname = 'h'),
      optly_flag_uint64("u64", .shortname = 'i'),
      optly_flag_float("f32", .shortname = 'j'),
      optly_flag_double("f64", .shortname = 'k')
    )
  );

  char *argv[] = ARGV("app", "--ch=Z", "--i8=-8", "--i16=-16", "--i32=-32", "--i64=-64", "--u8=8", "--u16=16", "--u32=32", "--u64=64", "--f32=1.5", "--f64=2.5");

  OptlyErrors errs = optly_parse_args(count_argc(argv), argv, &cmd);
  assert_err_count(&errs, 0);
  ASSERT_EQ_INT(optly_flag_value_char(&cmd, "ch"), 'Z');
  ASSERT_EQ_INT(optly_flag_value_int8(&cmd, "i8"), -8);
  ASSERT_EQ_INT(optly_flag_value_int16(&cmd, "i16"), -16);
  ASSERT_EQ_INT(optly_flag_value_int32(&cmd, "i32"), -32);
  ASSERT_EQ_INT(optly_flag_value_int64(&cmd, "i64"), -64);
  ASSERT_EQ_INT(optly_flag_value_uint8(&cmd, "u8"), 8);
  ASSERT_EQ_INT(optly_flag_value_uint16(&cmd, "u16"), 16);
  ASSERT_EQ_INT(optly_flag_value_uint32(&cmd, "u32"), 32);
  ASSERT_EQ_INT(optly_flag_value_uint64(&cmd, "u64"), 64);
  ASSERT_FLOAT_NEAR(optly_flag_value_float(&cmd, "f32"), 1.5f, 1e-6);
  ASSERT_FLOAT_NEAR(optly_flag_value_double(&cmd, "f64"), 2.5, 1e-9);
}

static void test_commands_and_command_flags(void) {
  OptlyCommand cmd = optly_command(
    "app",
    .flags = optly_flags(
      optly_flag_bool("verbose", .shortname = 'v')
    ),
    .commands = optly_commands(
      optly_command(
        "run",
        .flags = optly_flags(
          optly_flag_uint16("port", .shortname = 'p', .value.as_uint16 = 8080),
          optly_flag_bool("verbose", .shortname = 'v')
        )
      )
    )
  );

  char       *argv[] = ARGV("app", "--verbose", "run", "-p", "9000", "-v");
  OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
  assert_err_count(&errs, 0);
  ASSERT_TRUE(optly_flag_value_bool(&cmd, "verbose"));
  ASSERT_TRUE(cmd.next_command != NULL);
  ASSERT_EQ_STR(cmd.next_command->name, "run");
  ASSERT_EQ_INT(optly_flag_value_uint16(cmd.next_command, "port"), 9000);
  ASSERT_TRUE(optly_flag_value_bool(cmd.next_command, "verbose"));
}

static void test_subcommand_selection(void) {
  OptlyCommand cmd = optly_command(
    "app",
    .commands = optly_commands(
      optly_command(
        "run",
        .commands = optly_commands(optly_command("check", NULL))
      )
    )
  );

  char       *argv[] = ARGV("app", "run", "check");
  OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
  assert_err_count(&errs, 0);
  ASSERT_TRUE(cmd.next_command != NULL);
  ASSERT_EQ_STR(cmd.next_command->name, "run");
  ASSERT_TRUE(cmd.next_command->next_command != NULL);
  ASSERT_EQ_STR(cmd.next_command->next_command->name, "check");
}

static void test_positionals_and_delimiter(void) {
  OptlyCommand cmd = optly_command(
    "app",
    .flags = optly_flags(
      optly_flag_bool("verbose", .shortname = 'v')
    ),
    .positionals = optly_positionals(
      optly_positional("files", .min = 1, .max = 0)  // variadic
    )
  );

  char       *argv[] = ARGV("app", "--", "--not-a-flag", "a.txt", "b.txt");
  OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
  assert_err_count(&errs, 0);
  OptlyPositional *p = optly_get_positional(&cmd, "files");
  ASSERT_TRUE(p != NULL);
  ASSERT_EQ_INT(p->count, 3);
  ASSERT_EQ_STR(p->values[0], "--not-a-flag");
  ASSERT_EQ_STR(p->values[1], "a.txt");
  ASSERT_EQ_STR(p->values[2], "b.txt");
}

static void test_error_unknown_flag_missing_invalid(void) {
  OptlyCommand cmd = optly_command(
    "app",
    .flags = optly_flags(
      optly_flag_uint32("threads", .shortname = 't'),
      optly_flag_int32("num", .shortname = 'n')
    )
  );

  char       *argv[] = ARGV("app", "--unknown", "--threads", "--num=abc");
  OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
  assert_err_count(&errs, 3);
  assert_err_at(&errs, 0, OPTLY_ERR_UNKNOWN_FLAG, "--unknown");
  assert_err_at(&errs, 1, OPTLY_ERR_MISSING_VALUE, "threads");
  assert_err_at(&errs, 2, OPTLY_ERR_INVALID_VALUE, "abc");
}

static void test_error_unknown_command(void) {
  OptlyCommand cmd = optly_command(
    "app",
    .commands = optly_commands(optly_command("run", NULL))
  );

  char       *argv[] = ARGV("app", "build");
  OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
  assert_err_count(&errs, 1);
  assert_err_at(&errs, 0, OPTLY_ERR_UNKNOWN_COMMAND, "build");
}

static void test_error_required_and_batch_non_bool(void) {
  OptlyCommand cmd = optly_command(
    "app",
    .flags = optly_flags(
      optly_flag_string("token", .shortname = 'T', .required = true),
      optly_flag_uint32("threads", .shortname = 't')
    )
  );

  char       *argv[] = ARGV("app", "-tT");
  OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);

  // -tT => batch, but t is not bool => OPTLY_ERR_BATCH_NON_BOOL
  // T required token not found => OPTLY_ERR_MISSING_REQUIRED
  assert_err_count(&errs, 3);
  assert_err_at(&errs, 0, OPTLY_ERR_BATCH_NON_BOOL, "t");
  assert_err_at(&errs, 1, OPTLY_ERR_BATCH_NON_BOOL, "T");
  assert_err_at(&errs, 2, OPTLY_ERR_MISSING_REQUIRED, "token");
}

static void test_error_positionals_too_few_and_too_many(void) {
  // too few
  {
    OptlyCommand cmd = optly_command(
      "app",
      .positionals = optly_positionals(
        optly_positional("src", .min = 1, .max = 1)
      )
    );

    char       *argv[] = ARGV("app");
    OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
    assert_err_count(&errs, 1);
    assert_err_at(&errs, 0, OPTLY_ERR_POSITIONAL_TOO_FEW, "src");
  }
  // too many
  {
    OptlyCommand cmd = optly_command(
      "app",
      .positionals = optly_positionals(
        optly_positional("src", .min = 1, .max = 1),
        optly_positional("dst", .min = 0, .max = 1)
      )
    );

    char       *argv[] = ARGV("app", "a", "b", "c");
    OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
    assert_err_count(&errs, 1);
    assert_err_at(&errs, 0, OPTLY_ERR_POSITIONAL_TOO_MANY, "dst");
  }
}

static void test_enum_default_and_parse(void) {
  OptlyCommand cmd = {
    "app",
    .flags = optly_flags(
      optly_flag_enum("log", 'l', optly_enum_values("verbose", "debug", "verbose", "warn"))
    )
  };

  // No args -> default value
  {
    char       *argv[] = ARGV("app");
    OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
    assert_err_count(&errs, 0);
    ASSERT_EQ_STR(optly_flag_value_enum(&cmd, "log"), "verbose");
  }

  // Explicit value
  {
    char       *argv[] = ARGV("app", "--log=warn");
    OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
    assert_err_count(&errs, 0);
    ASSERT_EQ_STR(optly_flag_value_enum(&cmd, "log"), "warn");
  }
}

static void test_enum_errors(void) {
  OptlyCommand cmd = {
    "app",
    .flags = optly_flags(
      optly_flag_enum("mode", 'm', optly_enum_values("fast", "fast", "slow"))
    )
  };

  {
    char       *argv[] = ARGV("app", "--mode=invalid");
    OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
    assert_err_count(&errs, 1);
    assert_err_at(&errs, 0, OPTLY_ERR_INVALID_VALUE, "invalid");
  }

  {
    char       *argv[] = ARGV("app", "--mode");
    OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
    assert_err_count(&errs, 1);
    assert_err_at(&errs, 0, OPTLY_ERR_MISSING_VALUE, "mode");
  }
}

static void test_enum_short_and_overwrite(void) {
  OptlyCommand cmd = {
    "app",
    .flags = optly_flags(
      optly_flag_enum("level", 'l', optly_enum_values("low", "low", "mid", "high"))
    )
  };

  char       *argv[] = ARGV("app", "-l=mid", "-l", "high");
  OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);
  assert_err_count(&errs, 0);
  ASSERT_EQ_STR(optly_flag_value_enum(&cmd, "level"), "high");
}

static void test_enum_mixed_with_other_flags(void) {
  OptlyCommand cmd = {
    "app",
    .flags = optly_flags(
      optly_flag_enum("log", 'l', optly_enum_values("warn", "debug", "info", "warn")),
      optly_flag_uint32("threads", 't', .value.as_uint32 = 2)
    )
  };

  char       *argv[] = ARGV("app", "--log=info", "--threads", "8");
  OptlyErrors errs   = optly_parse_args(count_argc(argv), argv, &cmd);

  assert_err_count(&errs, 0);
  ASSERT_EQ_STR(optly_flag_value_enum(&cmd, "log"), "info");
  ASSERT_EQ_INT(optly_flag_value_uint32(&cmd, "threads"), 8);
}

int main(void) {
  fprintf(stderr, "\nRunning optly tests...\n\n");

  RUN_TEST(test_optional_flags_defaults);
  RUN_TEST(test_long_short_and_batch_bools);
  RUN_TEST(test_inline_and_separate_values);
  RUN_TEST(test_short_value_equals_and_space);
  RUN_TEST(test_typed_values);
  RUN_TEST(test_commands_and_command_flags);
  RUN_TEST(test_subcommand_selection);
  RUN_TEST(test_positionals_and_delimiter);
  RUN_TEST(test_error_unknown_flag_missing_invalid);
  RUN_TEST(test_error_unknown_command);
  RUN_TEST(test_error_required_and_batch_non_bool);
  RUN_TEST(test_error_positionals_too_few_and_too_many);
  RUN_TEST(test_enum_default_and_parse);
  RUN_TEST(test_enum_errors);
  RUN_TEST(test_enum_short_and_overwrite);
  RUN_TEST(test_enum_mixed_with_other_flags);

  fprintf(stderr, "\nAsserts: %d\n", g_asserts);

  if (g_failed == 0) {
    fprintf(stderr, ANSI_GREEN("✅ All tests passed\n"));
    return 0;
  }

  fprintf(stderr, ANSI_RED("❌ Failed: %d\n"), g_failed);
  return 1;
}
