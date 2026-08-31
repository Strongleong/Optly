// One flag per supported type, printed back so the .tspec pins the conversion.
#define OPTLY_IMPLEMENTATION
#include "optly.h"
#include <inttypes.h>
#include <stdio.h>

int main(int argc, char **argv) {
  OptlyCommand cmd = {
    .name  = "app",
    .flags = optly_flags(
      optly_flag_char("ch",   'c', "Char",   .value.as_char = '?'),
      optly_flag_int8("i8",   'a', "int8",   .value.as_int8 = 0),
      optly_flag_int16("i16", 'b', "int16",  .value.as_int16 = 0),
      optly_flag_int32("i32", 'd', "int32",  .value.as_int32 = 0),
      optly_flag_int64("i64", 'e', "int64",  .value.as_int64 = 0),
      optly_flag_uint8("u8",  'f', "uint8",  .value.as_uint8 = 0),
      optly_flag_uint16("u16",'g', "uint16", .value.as_uint16 = 0),
      optly_flag_uint32("u32",'h', "uint32", .value.as_uint32 = 0),
      optly_flag_uint64("u64",'i', "uint64", .value.as_uint64 = 0),
      optly_flag_float("f32", 'j', "float",  .value.as_float = 0),
      optly_flag_double("f64",'k', "double", .value.as_double = 0)
    )
  };

  optly_parse_args(argc, argv, &cmd);

  printf("ch=%c\n",  optly_flag_value_char(&cmd, "ch"));
  printf("i8=%d\n",  optly_flag_value_int8(&cmd, "i8"));
  printf("i16=%d\n", optly_flag_value_int16(&cmd, "i16"));
  printf("i32=%" PRId32 "\n", optly_flag_value_int32(&cmd, "i32"));
  printf("i64=%" PRId64 "\n", optly_flag_value_int64(&cmd, "i64"));
  printf("u8=%u\n",  optly_flag_value_uint8(&cmd, "u8"));
  printf("u16=%u\n", optly_flag_value_uint16(&cmd, "u16"));
  printf("u32=%" PRIu32 "\n", optly_flag_value_uint32(&cmd, "u32"));
  printf("u64=%" PRIu64 "\n", optly_flag_value_uint64(&cmd, "u64"));
  printf("f32=%.2f\n", (double)optly_flag_value_float(&cmd, "f32"));
  printf("f64=%.2f\n", optly_flag_value_double(&cmd, "f64"));
  return 0;
}
