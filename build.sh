#!/bin/sh


PROGNAME=$(basename $(pwd))
CFLAGS="-Wall -Wextra -std=c99"
CLIBS="-I./lib/"
CDEBUG="-O0 -ggdb -fsanitize=address -fno-omit-frame-pointer"
CPROFILE="-pg"
CRELEASE="-O3 -march=native"
CC="clang"

OUTDIR="./out"
VERBOSE=true
DEBUG=false

usage() {
  echo "-d --debug     Compile with debug flags"
  echo "-p --profile   Compile with profile flags"
  echo "-s --silent    Compile without unnececary output"
  echo "-o --outdir    Set output dir (default: ./out)"
  echo "-c --compiler  Set which compier to use (default: clang)"
  echo "-h --help      Print help"
}

handle_arg() {
  case "$1" in
    "${2}d"|"--debug") CFLAGS="$CFLAGS $CDEBUG"
     shift
     ;;
    "${2}p"|"--profile") CFLAGS="$CPROFILE $CFLAGS"
      shift
      ;;
    "${2}s"|"--silent") VERBOSE=false
     shift
     shift
     ;;
    "${2}o"|"--outdir") OUTDIR="$2"
     shift
     shift
     ;;
    "${2}c"|"--compiler") CC="$2"
     shift
     shift
     ;;
    "${2}h"|"--help") usage;
      shift
      exit 1;
      ;;
    ""|"-") # pass through
      shift
      ;;
    *) echo "Unknown command '$1'";
      usage;
      exit 1;
  esac
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -*)
      for (( i=0; i<${#1}; i++ )); do
        handle_arg "${1:$i:1}" ''
      done
      shift
      ;;
    *) echo "Unknown command '$1'";
      shift
      usage;
      exit 1;
  esac
done

set -e

if [ ! -d "$OUTDIR" ]; then
  mkdir -p "$OUTDIR";
fi

if $DEBUG; then
  CFLAGS="$CFLAGS $CDEBUG"
else
  CFLAGS="$CFLAGS $CRELEASE"
fi

sources=$(find ./src -name '*.c')

if $VERBOSE; then
  set -x
fi

$CC  $CFLAGS $CSTD $CLIBS -o "$OUTDIR/$PROGNAME" $sources ./lib/libs.c
$CC  $CFLAGS $CSTD $CLIBS -o "$OUTDIR/tests" ./tests/test.c
