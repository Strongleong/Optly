local dap = require('dap')
local dapui = require('dapui')

local cwd = vim.fn.getcwd();
local progname = vim.fn.fnamemodify(cwd, ':t')

dap.adapters.codelldb = {
  type = 'server',
  port = "${port}",
  executable = {
    command = 'codelldb',
    args = {"--port", "${port}"},
  }
}

dap.configurations.cpp = {
  {
    name = "Launch file",
    type = "codelldb",
    request = "launch",
    initCommands = { 'platform shell -- ' .. cwd .. '/build.sh -d' },
    program = cwd .. '/out/' .. progname,
    cwd = '${workspaceFolder}',
    stopOnEntry = false,
  },
}

dap.configurations.c = dap.configurations.cpp
dap.configurations.rust = dap.configurations.cpp

dapui.setup()
