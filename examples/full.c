#include <stdbool.h>
#include <stdio.h>

#define OPTLYARGS_IMPLEMENTATION
#include <optly.h>

static OptlyFlag flags[] = {
  { "verbose", 'v', "Enable verbose logging",           {false}, OPTLY_TYPE_BOOL },
  { "config",  'c', "Config file path",                 {.as_string = "nu blya"},    OPTLY_TYPE_STRING },
  { "env",     'e', "Environment (dev, staging, prod)", {.as_string = "dev"}, OPTLY_TYPE_STRING },
  { "json",    'j', "Output JSON",                      {false}, OPTLY_TYPE_BOOL },
  NULL_FLAG,
};

static OptlyCommand commands[] = {
  OPTLY_CMD(
      "build", "Build container images",
      {"tags",     't',  "Image tag",           {""},    OPTLY_TYPE_STRING},
      {"file",     'f',  "Dockerfile path",     {""},    OPTLY_TYPE_STRING},
      {"no-cache", 'n', "Disable build cache", {false}, OPTLY_TYPE_BOOL},   ),

  {"deploy",
    "Deploy service",
    (OptlyFlag[]){
      {"replicas", 'r', "Number of replicas",           {0}, OPTLY_TYPE_UINT32},
      {"wait",     'w', "Wait for deployment finishes", {0}, OPTLY_TYPE_BOOL},
      NULL_FLAG},
    (OptlyCommand[]){
      OPTLY_CMD("rollback", "Rollback service",
          {"revision",
          'r',
          "Revision to rollabck to",
          {0},
          OPTLY_TYPE_UINT32}, ),

      OPTLY_CMD("status", "Get status of deployed service",),
      NULL_COMMAND,
    },
    NULL,
    {0}},

  OPTLY_CMD(
      "logs", "View service logs",
      {"follow", 'f', "Follow log output",          {false}, OPTLY_TYPE_BOOL},
      {"lines",  'n', "Number of lines ",           {20},    OPTLY_TYPE_UINT32},
      {"since",  's', "Show logs scinse timestamp", {0},     OPTLY_TYPE_UINT32}, ),

  {
    "service",
    "Manage services",
    NULL,
    (OptlyCommand[]){
      OPTLY_CMD("start",   NULL,
          { "scale", 's', "Start N instances", {1}, OPTLY_TYPE_UINT32 },),
      OPTLY_CMD("stop",    NULL,),
      OPTLY_CMD("restart", NULL,),
      NULL_COMMAND,
    },
    NULL,
    {0}
  },

  NULL_COMMAND,
};

void build(void) {
  printf("build\n");
}

void deploy(void) {
  printf("deploy\n");
}

void logs(void) {
  printf("logs\n");
}

void services(void) {
  printf("services\n");
}

int main(int argc, char *argv[]) {
  OptlyCommand cmd = optly_parse_args(argc, argv, flags, commands);

  bool verbose = cmd.flags[0].value.as_bool;
  char *config = cmd.flags[1].value.as_string;
  char *env = cmd.flags[2].value.as_string;
  bool json = flags[3].value.as_bool;

  printf("Global flags:\n");
  printf("verbose = %s\n", verbose ? "true" : "false");
  printf("config = %s\n", config);
  printf("env = %s\n", env);
  printf("json = %s\n", json ? "true" : "false");

  if (cmd.next_command == &commands[0]) {
    build();
  } else if (cmd.next_command == &commands[1]) {
    deploy();
  } else if (cmd.next_command == &commands[2]) {
    logs();
  } else if (cmd.next_command == &commands[3]) {
    services();
  } else {
    optly_usage(cmd.name, cmd.commands, cmd.flags);
  }
}
