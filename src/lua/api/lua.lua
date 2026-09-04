local raw = trxc.lua
local api = trx.api

api.module("lua", {
  order = 36,
  description = "Evaluating Lua at runtime: a string of code, or a file on disk. "
    .. "Both run in the same state as every other script.",
})

-- The bridge reports a failure as two values; the public shape is one.
local function wrap(kind, message)
  if kind == nil then
    return nil
  end
  return { kind = kind, message = message }
end

api.type("lua.Error", {
  record = true,
  description = "What went wrong while running Lua.",
  fields = {
    kind = {
      type = "string",
      description = 'Either `"syntax"` or `"runtime"`.',
    },
    message = { type = "string", description = "The error text." },
  },
})

api.define("lua.eval_expr", {
  description = "Evaluates a string of Lua code, as the `/lua` console command does.",
  params = {
    {
      name = "code",
      type = "string",
      description = "Any chunk of Lua, not only an expression.",
    },
  },
  returns = {
    type = "lua.Error",
    nullable = true,
    description = "`nil` when the code ran to completion, and what went wrong otherwise. A "
      .. "failure comes back as a value rather than raising, so the caller decides what it "
      .. "means.",
  },
  examples = { [[trx.lua.eval_expr("trx.console.log('hello')")]] },
  impl = function(code)
    return wrap(raw.eval_expr(code))
  end,
})

api.define("lua.eval_file", {
  description = "Runs a Lua file, the way a level script is run. A file that cannot be "
    .. 'read reports as a `"runtime"` failure.',
  params = {
    { name = "path", type = "string", description = "Path of the file." },
  },
  returns = {
    type = "lua.Error",
    nullable = true,
  },
  examples = { [[trx.lua.eval_file("data/ship/scripts/extra.lua")]] },
  impl = function(path)
    return wrap(raw.eval_file(path))
  end,
})
