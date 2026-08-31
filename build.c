// Build tool for optly. Compile it once and it rebuilds itself from then on:
//
//   cc build.c -o build
//   ./build              # compile every example into ./out
//   ./build tests        # run the .tspec suite
//
// It parses its own command line with optly, so the library is exercised by
// the thing that builds it.

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <process.h>
#include <windows.h>

// NOTE: windows.h defines ERROR, which collides with the log level names.
#ifdef ERROR
#undef ERROR
#endif

#define mkdir(path, mode) _mkdir(path)
#define PATH_SEP          "\\"
#define PATH_LIST_SEP     ';'
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PATH_SEP      "/"
#define PATH_LIST_SEP ':'
#endif  // _WIN32

#define PATH_MAX_LEN 4096
#define CMD_MAX_ARGV 64
#define MAX_ENTRIES  128

// NOTE: strum is the reference .tspec runner, but the format is not tied to
// it. --tspec-runner exists so a second implementation can be dropped in
// without touching this file.
#define DEFAULT_TSPEC_RUNNER "strum"
#define STRUM_HOMEPAGE       "https://github.com/Strongleong/Strum"

#define OPTLY_GEN_HELP_FLAG
#define OPTLY_GEN_HELP_COMMAND
#define OPTLY_IMPLEMENTATION
#include "optly.h"

static bool verbose = true;

#define LOG(level, ...)                     \
  do {                                      \
    fprintf(stderr, "[build] " level ": "); \
    fprintf(stderr, __VA_ARGS__);           \
    fprintf(stderr, "\n");                  \
  } while (0)

#define INFO(...)                          \
  do {                                     \
    if (verbose) LOG("INFO", __VA_ARGS__); \
  } while (0)
#define WARN(...)  LOG("WARN", __VA_ARGS__)
#define FATAL(...) LOG("FATAL", __VA_ARGS__)

// NOTE: the tree is built inside main rather than at file scope. The DSL
// expands to compound literals, which are not constant initializers, so a
// static one does not compile under -pedantic. The header says as much.
static OptlyCommand command;

// ---------------------------------------------------------------- filesystem

// snprintf silently truncates; a build tool that half-writes a path would
// compile to the wrong file. Every path is joined through here so overflow is
// reported instead of guessed at.
static bool join_path(char *out, size_t cap, const char *dir, const char *name) {
  int n = snprintf(out, cap, "%s" PATH_SEP "%s", dir, name);
  return n > 0 && (size_t)n < cap;
}

static bool dir_exists(const char *path) {
  struct stat sb;
  return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}

static bool file_exists(const char *path) {
  struct stat sb;
  return stat(path, &sb) == 0 && S_ISREG(sb.st_mode);
}

// Collects the names of entries in dir that pass filter, sorted, so a build
// does not depend on the order the filesystem hands them back.
static int compare_names(const void *a, const void *b) {
  return strcmp(*(const char *const *)a, *(const char *const *)b);
}

static int list_dir(const char *dir, bool (*filter)(const char *name), char names[][256], int max) {
  DIR *d = opendir(dir);

  if (!d) {
    FATAL("Can not open %s: %s", dir, strerror(errno));
    return -1;
  }

  int count = 0;

  for (struct dirent *e = readdir(d); e && count < max; e = readdir(d)) {
    if (e->d_name[0] == '.') {
      continue;
    }

    if (filter && !filter(e->d_name)) {
      continue;
    }

    snprintf(names[count], 256, "%s", e->d_name);
    count++;
  }

  closedir(d);

  char *ptrs[MAX_ENTRIES];

  for (int i = 0; i < count; i++) {
    ptrs[i] = names[i];
  }

  qsort(ptrs, (size_t)count, sizeof(ptrs[0]), compare_names);

  char sorted[MAX_ENTRIES][256];

  for (int i = 0; i < count; i++) {
    snprintf(sorted[i], 256, "%s", ptrs[i]);
  }

  for (int i = 0; i < count; i++) {
    snprintf(names[i], 256, "%s", sorted[i]);
  }

  return count;
}

// ------------------------------------------------------------------ processes

static bool is_executable(const char *path) {
#ifdef _WIN32
  return _access(path, 0) == 0;
#else
  return access(path, X_OK) == 0;
#endif
}

// Resolves a bare name against PATH so a missing runner is reported before
// anything is compiled, rather than as a failed exec halfway through.
static bool find_executable(const char *name, char *out, size_t cap) {
  if (strchr(name, PATH_SEP[0]) != NULL) {
    snprintf(out, cap, "%s", name);
    return is_executable(out);
  }

  const char *path = getenv("PATH");

  if (!path) {
    return false;
  }

  while (*path) {
    const char *sep = strchr(path, PATH_LIST_SEP);
    size_t      len = sep ? (size_t)(sep - path) : strlen(path);

    if (len > 0 && len < cap) {
      snprintf(out, cap, "%.*s" PATH_SEP "%s", (int)len, path, name);

      if (is_executable(out)) {
        return true;
      }
    }

    if (!sep) {
      break;
    }

    path = sep + 1;
  }

  return false;
}

static void print_command(char *const argv[]) {
  if (!verbose) {
    return;
  }

  fprintf(stderr, "+");

  for (int i = 0; argv[i]; i++) {
    fprintf(stderr, " %s", argv[i]);
  }

  fprintf(stderr, "\n");
}

// Runs argv to completion and returns its exit status, or -1 if it could not
// be started. No shell is involved, so arguments never need quoting.
static int run(char *const argv[]) {
  print_command(argv);

#ifdef _WIN32
  intptr_t rc = _spawnvp(_P_WAIT, argv[0], (const char *const *)argv);
  return rc < 0 ? -1 : (int)rc;
#else
  pid_t pid = fork();

  if (pid < 0) {
    FATAL("Can not fork: %s", strerror(errno));
    return -1;
  }

  if (pid == 0) {
    execvp(argv[0], argv);
    fprintf(stderr, "[build] FATAL: can not run %s: %s\n", argv[0], strerror(errno));
    _exit(127);
  }

  int status = 0;

  if (waitpid(pid, &status, 0) < 0) {
    FATAL("Can not wait for %s: %s", argv[0], strerror(errno));
    return -1;
  }

  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }

  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

// ---------------------------------------------------------------- the targets

static char outdir[PATH_MAX_LEN] = {0};

static bool has_c_extension(const char *name) {
  size_t len = strlen(name);
  return len > 2 && strcmp(name + len - 2, ".c") == 0;
}

static bool build_example(const char *cc, const char *std, bool debug, const char *name) {
  char src[PATH_MAX_LEN];
  char out[PATH_MAX_LEN];
  char stdflag[64];

  char stem[256];

  snprintf(stdflag, sizeof(stdflag), "-std=%s", std);
  snprintf(stem, sizeof(stem), "%s", name);
  stem[strlen(stem) - 2] = '\0';  // drop ".c"

  if (!join_path(src, sizeof(src), "examples", name) || !join_path(out, sizeof(out), outdir, stem)) {
    FATAL("Path too long for %s", name);
    return false;
  }

  char *argv[CMD_MAX_ARGV];
  int   i = 0;

  argv[i++] = (char *)cc;
  argv[i++] = stdflag;
  argv[i++] = (char *)"-Wall";
  argv[i++] = (char *)"-Wextra";
  argv[i++] = (char *)"-Werror";
  argv[i++] = (char *)"-pedantic";
  argv[i++] = (char *)"-I.";

  if (debug) {
    argv[i++] = (char *)"-O0";
    argv[i++] = (char *)"-g";
  } else {
    argv[i++] = (char *)"-O2";
  }

  argv[i++] = src;
  argv[i++] = (char *)"-o";
  argv[i++] = out;
  argv[i]   = NULL;

  return run(argv) == 0;
}

static bool build_examples(const char *cc, const char *std, bool debug) {
  char names[MAX_ENTRIES][256];
  int  count = list_dir("examples", has_c_extension, names, MAX_ENTRIES);

  if (count < 0) {
    return false;
  }

  if (count == 0) {
    WARN("No examples found");
    return true;
  }

  bool ok = true;

  for (int i = 0; i < count; i++) {
    if (!build_example(cc, std, debug, names[i])) {
      FATAL("Failed to build examples" PATH_SEP "%s", names[i]);
      ok = false;
    }
  }

  if (ok) {
    INFO("Built %d example%s into %s", count, count == 1 ? "" : "s", outdir);
  }

  return ok;
}

static bool run_tests(const char *runner_name) {
  char runner[PATH_MAX_LEN];

  if (!find_executable(runner_name, runner, sizeof(runner))) {
    FATAL("Test runner '%s' not found in PATH", runner_name);
    FATAL("strum is the reference .tspec runner: " STRUM_HOMEPAGE);
    FATAL("Use --tspec-runner to point at a different one");
    return false;
  }

  if (!dir_exists("tests")) {
    FATAL("No tests directory here");
    return false;
  }

  char *argv[] = {runner, (char *)".", NULL};

  if (chdir("tests") != 0) {
    FATAL("Can not enter tests: %s", strerror(errno));
    return false;
  }

  int rc = run(argv);

  if (chdir("..") != 0) {
    FATAL("Can not leave tests: %s", strerror(errno));
    return false;
  }

  return rc == 0;
}

static bool clean(void) {
  char names[MAX_ENTRIES][256];
  int  count = list_dir(outdir, NULL, names, MAX_ENTRIES);

  if (count < 0) {
    return false;
  }

  for (int i = 0; i < count; i++) {
    char path[PATH_MAX_LEN];

    if (!join_path(path, sizeof(path), outdir, names[i])) {
      WARN("Path too long for %s", names[i]);
      continue;
    }

    if (remove(path) != 0) {
      WARN("Can not remove %s: %s", path, strerror(errno));
    }
  }

  INFO("Removed %d file%s from %s", count, count == 1 ? "" : "s", outdir);
  return true;
}

int main(int argc, char *argv[]) {
  command = (OptlyCommand){
    .name        = "build",
    .description = "Build optly's examples and run its tests",
    .flags       = optly_flags(
      optly_flag_bool("debug", 'd', "Compile with debug flags", .value.as_bool = false),
      optly_flag_bool("silent", 's', "Compile without unnecessary output", .value.as_bool = false),
      optly_flag_string("outdir", 'o', "Set output dir", .value.as_string = "." PATH_SEP "out"),
      optly_flag_string("compiler", 'c', "Set which C compiler to use", .value.as_string = "cc"),
      optly_flag_string("std", 0, "Set which C standard to compile against", .value.as_string = "c99")
    ),
    .commands = optly_commands(
      optly_command("tests", "Run the .tspec suite", .flags = optly_flags(optly_flag_string("tspec-runner", 'r', "Runner to execute .tspec files with", .value.as_string = DEFAULT_TSPEC_RUNNER))),
      optly_command("clean", "Remove the output directory's contents")
    )
  };

  optly_parse_args(argc, argv, &command);

  verbose = !optly_flag_value_bool(&command, "silent");

  snprintf(outdir, sizeof(outdir), "%s", optly_flag_value_string(&command, "outdir"));

  size_t len = strlen(outdir);

  while (len > 1 && outdir[len - 1] == PATH_SEP[0]) {
    outdir[--len] = '\0';
  }

  if (!dir_exists(outdir) && mkdir(outdir, 0755) != 0) {
    FATAL("Can not create %s: %s", outdir, strerror(errno));
    return 1;
  }

  if (optly_is_command(command.next_command, "tests")) {
    const char *runner = optly_flag_value_string(command.next_command, "tspec-runner");
    return run_tests(runner) ? 0 : 1;
  }

  if (optly_is_command(command.next_command, "clean")) {
    return clean() ? 0 : 1;
  }

  if (!file_exists("optly.h")) {
    FATAL("optly.h is not here; run build from the repository root");
    return 1;
  }

  return build_examples(
           optly_flag_value_string(&command, "compiler"),
           optly_flag_value_string(&command, "std"),
           optly_flag_value_bool(&command, "debug")
         )
           ? 0
           : 1;
}
