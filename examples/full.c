#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define OPTLY_GEN_HELP_FLAG
#define OPTLY_GEN_HELP_COMMAND
#define OPTLY_IMPLEMENTATION
#include <optly.h>

// Full CLI example that demonstrates:
// - global flags
// - command flags
// - nested subcommands
// - positional arguments
static OptlyCommand cmd = optly_command(
  "optly-demo",
  "Full example of Optly usage",

  // Global flags parsed before command selection.
  optly_flags(
    optly_flag_bool("verbose", 'v', "Enable verbose logging"),
    optly_flag_string("config", 'c', "Config file path", .value.as_string = "./config"),
    optly_flag_string("env", 'e', "Environment (dev, staging, prod)", .value.as_string = "dev"),
    optly_flag_bool("json", 'j', "Output JSON")
  ),

  // Top-level commands.
  optly_commands(
    optly_command(
      "build",
      "Build container images",
      optly_flags(
        optly_flag_string("tag", 't', "Image tag", .required = true),
        optly_flag_string("file", 'f', "Dockerfile path"),
        optly_flag_bool("no-cache", .description = "Disable build cache")
      ),
      .positionals = optly_positionals(optly_positional("context", "Build context directory", 1, 1))
    ),

    optly_command(
      "deploy",
      "Deploy service",
      optly_flags(
        optly_flag_uint32("replicas", 'r', "Number of replicas", .value.as_uint32 = 1),
        optly_flag_bool("wait", 'w', "Wait until deployment finishes")
      ),
      optly_commands(
        optly_command("status", "Get deployment status"),
        optly_command("rollback", "Rollback service", optly_flags(optly_flag_uint32("revision", 'r', "Revision to rollback to")))
      ),
      optly_positionals(optly_positional("service", "Service name", 1, 1))
    ),

    optly_command(
      "logs",
      "View service logs",
      optly_flags(
        optly_flag_bool("follow", 'f', "Follow output"),
        optly_flag_uint32("lines", 'n', "Number of lines", .value.as_uint32 = 20),
        optly_flag_uint32("since", 's', "Show logs since unix timestamp")
      ),
      .positionals = optly_positionals(optly_positional("services", "Services to read logs from", 0, 0))
    )
  )
);

static void do_build(OptlyCommand *c) {
  printf("build.tag      = %s\n", optly_flag_value_string(c, "tag"));
  printf("build.file     = %s\n", optly_flag_value_string(c, "file"));
  printf("build.no-cache = %s\n", optly_flag_value_bool(c, "no-cache") ? "true" : "false");

  OptlyPositional *context = optly_get_positional(c, "context");
  if (context && context->count == 1) {
    printf("build.context  = %s\n", context->values[0]);
  }
}

static void do_deploy(OptlyCommand *c) {
  printf("deploy.replicas = %u\n", optly_flag_value_uint32(c, "replicas"));
  printf("deploy.wait     = %s\n", optly_flag_value_bool(c, "wait") ? "true" : "false");

  OptlyPositional *service = optly_get_positional(c, "service");
  if (service && service->count == 1) {
    printf("deploy.service  = %s\n", service->values[0]);
  }

  if (optly_is_command(c->next_command, "status")) {
    puts("deploy.subcommand = status");
  } else if (optly_is_command(c->next_command, "rollback")) {
    printf("deploy.rollback.revision = %u\n", optly_flag_value_uint32(c->next_command, "revision"));
  }
}

static void do_logs(OptlyCommand *c) {
  printf("logs.follow = %s\n", optly_flag_value_bool(c, "follow") ? "true" : "false");
  printf("logs.lines  = %u\n", optly_flag_value_uint32(c, "lines"));
  printf("logs.since  = %u\n", optly_flag_value_uint32(c, "since"));

  OptlyPositional *services = optly_get_positional(c, "services");
  if (services && services->count > 0) {
    for (size_t i = 0; i < services->count; i++) {
      printf("logs.service[%zu] = %s\n", i, services->values[i]);
    }
  }
}

int main(int argc, char *argv[]) {
  OptlyErrors errs = optly_parse_args(argc, argv, &cmd);

  if (errs.count > 0) {
    for (size_t i = 0; i < errs.count; i++) {
      fprintf(stderr, "ERR: %s (%s)\n", optly_error_message(errs.items[i].kind), errs.items[i].arg ? errs.items[i].arg : "-");
    }
    return 1;
  }

  printf("global.verbose = %s\n", optly_flag_value_bool(&cmd, "verbose") ? "true" : "false");
  printf("global.config  = %s\n", optly_flag_value_string(&cmd, "config"));
  printf("global.env     = %s\n", optly_flag_value_string(&cmd, "env"));
  printf("global.json    = %s\n", optly_flag_value_bool(&cmd, "json") ? "true" : "false");

  if (optly_is_command(cmd.next_command, "build")) {
    do_build(cmd.next_command);
  } else if (optly_is_command(cmd.next_command, "deploy")) {
    do_deploy(cmd.next_command);
  } else if (optly_is_command(cmd.next_command, "logs")) {
    do_logs(cmd.next_command);
  } else {
    optly_usage(&cmd);
  }

  return 0;
}
