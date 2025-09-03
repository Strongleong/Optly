#define OPTLYARGS_IMPLEMENTATION
#include <optly.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* ---------- helpers ---------- */

#define RUN_TEST(fn) do { \
    /* Tests are fast, so if test would pass it would override */ \
    /* word FAIL instantly, but if test would fail then FAIL stay */ \
    printf("\033[31m[ FAIL ]\033[0m %s\n", #fn); \
    fn(); \
    printf("\033[F\033[32m[ PASS ]\033[0m %s\n", #fn); \
} while(0)

static void reset_flags(OptlyFlag *f) {
    for (int i = 0; !optly_is_flag_null(&f[i]); ++i) {
        switch (f[i].type) {
            case OPTLY_TYPE_BOOL:   f[i].value.as_bool   = false; break;
            case OPTLY_TYPE_STRING: f[i].value.as_string = NULL;  break;
            case OPTLY_TYPE_INT64:  f[i].value.as_int64  = 0;     break;
            case OPTLY_TYPE_UINT64: f[i].value.as_uint64 = 0;     break;
            default: memset(&f[i].value, 0, sizeof(f[i].value));  break;
        }
    }
}

/* call optly__parse_flags the same way the parser does:
   pass a cursor (char**) by address, and a mutable argc */
static void call_parse(char **vec, int len, OptlyFlag *flags) {
    char **cursor = vec;   /* <- real char** */
    int    argc   = len;
    optly__parse_flags(&cursor, &argc, flags);
}

/* ---------- flag set used in tests ---------- */

static OptlyFlag g_flags[] = {
    { "help",    'h', {0}, OPTLY_TYPE_BOOL   },
    { "verbose", 'v', {0}, OPTLY_TYPE_BOOL   },
    { "value",   'x', {0}, OPTLY_TYPE_INT64  },
    { "name",    'n', {0}, OPTLY_TYPE_STRING },
    NULL_FLAG
};

/* ---------- tests ---------- */

static void test_long_bool_plain(void) {
    reset_flags(g_flags);
    char *vec[] = { "--help", NULL };
    call_parse(vec, 1, g_flags);
    assert(g_flags[0].value.as_bool == true);
}

static void test_short_bool_plain(void) {
    reset_flags(g_flags);
    char *vec[] = { "-v", NULL };
    call_parse(vec, 1, g_flags);
    assert(g_flags[1].value.as_bool == true);
}

static void test_long_bool_equals_true_false(void) {
    reset_flags(g_flags);
    char *vec1[] = { "--verbose=true", NULL };
    call_parse(vec1, 1, g_flags);
    assert(g_flags[1].value.as_bool == true);

    reset_flags(g_flags);
    char *vec2[] = { "--verbose=false", NULL };
    call_parse(vec2, 1, g_flags);
    assert(g_flags[1].value.as_bool == false);
}

static void test_short_bool_space_yes_no(void) {
    reset_flags(g_flags);
    char *vec1[] = { "-v", "yes", NULL };
    call_parse(vec1, 2, g_flags);
    assert(g_flags[1].value.as_bool == true);

    reset_flags(g_flags);
    char *vec2[] = { "-v", "no", NULL };
    call_parse(vec2, 2, g_flags);
    assert(g_flags[1].value.as_bool == false);
}

static void test_short_bool_abbrev_and_digits(void) {
    reset_flags(g_flags);
    char *vec1[] = { "--verbose", "y", NULL };
    call_parse(vec1, 2, g_flags);
    assert(g_flags[1].value.as_bool == true);

    reset_flags(g_flags);
    char *vec2[] = { "--verbose", "n", NULL };
    call_parse(vec2, 2, g_flags);
    assert(g_flags[1].value.as_bool == false);

    reset_flags(g_flags);
    char *vec3[] = { "--verbose", "1", NULL };
    call_parse(vec3, 2, g_flags);
    assert(g_flags[1].value.as_bool == true);

    reset_flags(g_flags);
    char *vec4[] = { "--verbose", "0", NULL };
    call_parse(vec4, 2, g_flags);
    assert(g_flags[1].value.as_bool == false);
}

static void test_batch_short_no_values(void) {
    reset_flags(g_flags);
    char *vec[] = { "-hv", NULL };
    call_parse(vec, 1, g_flags);
    assert(g_flags[0].value.as_bool == true);
    assert(g_flags[1].value.as_bool == true);
}

static void test_batch_short_equals_errors(void) {
    reset_flags(g_flags);
    /* The parser should treat "-hv=1" as an error and not set either flag */
    char *vec[] = { "-hv=1", NULL };
    call_parse(vec, 1, g_flags);
    assert(g_flags[0].value.as_bool == false);
    assert(g_flags[1].value.as_bool == false);
}

static void test_long_value_equals_and_space(void) {
    reset_flags(g_flags);
    char *vec1[] = { "--value=15", NULL };
    call_parse(vec1, 1, g_flags);
    assert(g_flags[2].value.as_int64 == 15);

    reset_flags(g_flags);
    char *vec2[] = { "--value", "42", NULL };
    call_parse(vec2, 2, g_flags);
    assert(g_flags[2].value.as_int64 == 42);
}

static void test_short_value_equals_and_space(void) {
    /* -x is short for "value" in this set */
    reset_flags(g_flags);
    char *vec1[] = { "-x=77", NULL };
    call_parse(vec1, 1, g_flags);
    assert(g_flags[2].value.as_int64 == 77);

    reset_flags(g_flags);
    char *vec2[] = { "-x", "99", NULL };
    call_parse(vec2, 2, g_flags);
    assert(g_flags[2].value.as_int64 == 99);
}

static void test_string_value(void) {
    reset_flags(g_flags);
    char *vec1[] = { "--name=Alice", NULL };
    call_parse(vec1, 1, g_flags);
    assert(g_flags[3].value.as_string && strcmp(g_flags[3].value.as_string, "Alice") == 0);

    reset_flags(g_flags);
    char *vec2[] = { "--name", "Bob", NULL };
    call_parse(vec2, 2, g_flags);
    assert(g_flags[3].value.as_string && strcmp(g_flags[3].value.as_string, "Bob") == 0);
}

static void test_unknown_flag_is_ignored(void) {
    reset_flags(g_flags);
    char *vec[] = { "--doesnotexist", NULL };
    call_parse(vec, 1, g_flags);
    /* No crashes; nothing set */
    assert(g_flags[0].value.as_bool == false);
    assert(g_flags[1].value.as_bool == false);
    assert(g_flags[2].value.as_int64 == 0);
}

int main(void) {
    printf("\n\033[1;37mRunning optly__parse_flags tests...\033[0m\n\n");

    RUN_TEST(test_long_bool_plain);
    RUN_TEST(test_short_bool_plain);
    RUN_TEST(test_long_bool_equals_true_false);
    RUN_TEST(test_short_bool_space_yes_no);
    RUN_TEST(test_short_bool_abbrev_and_digits);
    RUN_TEST(test_batch_short_no_values);
    RUN_TEST(test_batch_short_equals_errors);
    RUN_TEST(test_long_value_equals_and_space);
    RUN_TEST(test_short_value_equals_and_space);
    RUN_TEST(test_string_value);
    RUN_TEST(test_unknown_flag_is_ignored);

    printf("\n\033[1;32m✅ All tests passed!\033[0m\n");
    return 0;
}
