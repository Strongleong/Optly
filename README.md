# C PROJECT TEMPLATE

This is my simple template for writing C projects.

## How to run

```console
git clone https://github.com/strongleong/ctemplate
cd ctemplate
./build.sh && ./out/ctemplate
```

## `./build.sh`

I use plain bash script `./build.sh` as "build system" with couple flags that may be useful sometimes (all flags that `./build.sh` supports I actually used)

Build flags:

```console
-d --debug     Compile with debug flags
-p --profile   Compile with profile flags
-s --silent    Compile without unnececary output
-o --outdir    Set output dir (default: ./out)
-c --compiler  Set which compier to use (default: clang)
-h --help      Print help
```

By default this script includes headers from `./lib/` folder, serch every `*.c` file in `./src` folder and compiles them into `./out/<PROJECT_FOLDER_NAME>`

If you want to chage output file name or compile multiple binaries just change the `./build.sh` script lol.

## `./dap.lua`

`dap.lua` file sets up `codelldb` debug adapter  for nvim (see (nvim DAP)[https://github.com/mfussenegger/nvim-dap])

If you use `clangd` you can run `bear -- ./build.sh` to generate `compile_commands.json`. If you changed copmiler flags you should regenrate this file.
