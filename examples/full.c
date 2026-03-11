#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define OPTLY_IMPLEMENTATION
#include <optly.h>

static OptlyCommand cmd = optly_command(
  "full.c",
  "Full example of optly usage",

  optly_flags(
    optly_flag_bool("verbose", 'v', "Enable verbose logging"),
    optly_flag_string("config", 'c', "Config file path", .value.as_string = "./config"),
    optly_flag_string("env", 'e', "Environment (dev, staging, prod)", .value.as_string = "dev"),
    optly_flag_bool("json", 'j', "Output JSON")
  ),

  optly_commands(
    optly_command(
      "build",
      "Build container images",
      optly_flags(
        optly_flag_string("tags", 't', "Image tag"),
        optly_flag_string("file", 'f', "Dockerfile path"),
        // NOTE: Here we skipped shortname. -n would not work but --no-cache will
        optly_flag_bool("no-cache", .description = "Disable build cache")
      )
    ),

    optly_command(
      "deploy",
      "Deploy servie",
      optly_flags(
        optly_flag_uint32("replicas", 'r', "Number of replicas"),
        optly_flag_bool("wait", 'w', "Wait for deployment finishes")
      ),
      optly_commands(
        optly_command("status", "Get status of deployed service"),
        optly_command(
          "rollback",
          "Rollback service",
          // NOTE: Flags for subcommand of command
          optly_flags(
            optly_flag_uint32("revision", 'r', "Revision to rollabck to")
          )
        )
      )
    ),

    optly_command(
      "deploy",
      "Deploy service",
      optly_flags(
        optly_flag_uint32("replicas", 'r', "Number of replicas"),
        optly_flag_bool("wait", 'w', "Wait for deployment finishes")
      ),
      optly_commands(
        optly_command("status", "Get status of deployed service")
      )
    ),

    optly_command(
      "rollback",
      "Rollback service",
      optly_flags(
        optly_flag("revision", 'r', "Revision to rollabck to")
      )
    ),

    optly_command(
      "logs",
      "View service logs",
      optly_flags(
        optly_flag_bool("follow", 'f', "Follow log output"),
        optly_flag_uint32("lines", 'n', "Number of lines ", .value.as_uint32 = 20),
        optly_flag_uint32("since", 's', "Show logs scinse timestamp")
      )
    ),

    optly_command(
      "service",
      "Manage services",
      // NOTE: Here we don't have any flags so we skip then to subcommands
      .commands = optly_commands(
        optly_command(
          "start",
          .flags = optly_flags(
            optly_flag_uint32("scale", 's', "Start N instances", .value.as_uint32 = 1)
          )
        ),
        optly_command("stop", NULL),
        optly_command("restart", NULL)
      )
    )
  )
);

void build(OptlyCommand *cmd) {
  printf("Build command flags:\n");
  printf("tags     = %s\n", optly_string(cmd, "tags"));
  printf("file     = %s\n", optly_string(cmd, "file"));
  printf("no-cache = %s\n", optly_bool(cmd, "no-cache") ? "true" : "false");
}

void rollback(OptlyCommand *cmd) {
  printf("Rollback command flags:\n");
  printf("revision = %d\n", optly_uint32(cmd, "revision"));
}

void status(OptlyCommand *cmd) {
  printf("Status command have no flags.\n");
  (void)cmd;
}

void deploy(OptlyCommand *cmd) {
  printf("Logs command flags:\n");
  printf("replicas = %d\n", optly_uint32(cmd, "replicas"));
  printf("wait     = %s\n", optly_bool(cmd, "wait") ? "true" : "false");

  if (optly_is_command(cmd->next_command, "rollback")) {
    rollback(cmd->next_command);
  } else if (optly_is_command(cmd->next_command, "status")) {
    status(cmd->next_command);
  } else {
    optly_usage(cmd->name, cmd->commands, cmd->flags);
  }
}

void logs(OptlyCommand *cmd) {
  printf("Logs command flags:\n");
  printf("follow   = %s\n", optly_bool(cmd, "follow") ? "true" : "false");
  printf("lines    = %d\n", optly_uint32(cmd, "lines"));
  printf("since    = %d\n", optly_uint32(cmd, "since"));
}

void start(OptlyCommand *cmd) {
  printf("Start command flags:\n");
  printf("scale = %d\n", optly_uint32(cmd, "scale"));
}

void stop(OptlyCommand *cmd) {
  printf("Stop command have no flags.\n");
  (void)cmd;
}

void restart(OptlyCommand *cmd) {
  printf("Restart command have no flags.\n");
  (void)cmd;
}

void services(OptlyCommand *cmd) {
  printf("Service command have no flags.\n");

  if (optly_is_command(cmd->next_command, "start")) {
    start(cmd->next_command);
  } else if (optly_is_command(cmd->next_command, "stop")) {
    stop(cmd->next_command);
  } else if (optly_is_command(cmd->next_command, "restart")) {
    restart(cmd->next_command);
  } else {
    optly_usage(cmd->name, cmd->commands, cmd->flags);
  }
}

int main(int argc, char *argv[]) {
  optly_parse_args(argc, argv, &cmd);

  printf("Global flags:\n");
  printf("verbose = %s\n", optly_bool(&cmd, "verbose") ? "true" : "false");
  printf("config  = %s\n", optly_string(&cmd, "config"));
  printf("env     = %s\n", optly_string(&cmd, "env"));
  printf("json    = %s\n", optly_bool(&cmd, "json") ? "true" : "false");

  if (optly_is_command(cmd.next_command, "build")) {
    build(cmd.next_command);
  } else if (optly_is_command(cmd.next_command, "deploy")) {
    deploy(cmd.next_command);
  } else if (optly_is_command(cmd.next_command, "logs")) {
    logs(cmd.next_command);
  } else if (optly_is_command(cmd.next_command, "services")) {
    services(cmd.next_command);
  } else if (optly_is_command(cmd.next_command, "stop")) {
    printf("this will never reach it here, 'stop' command is subcommand of 'service' command\n");
  } else {
    optly_usage(cmd.name, cmd.commands, cmd.flags);
  }

  return 0;
}
