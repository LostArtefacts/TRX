-- Evaluates a string of Lua code.
--
-- Usages:
--   /lua trx.lara.item.hit_points = 100

trx.locale.declare({
  ["console/cmd/lua/help"] = "Executes the given Lua code string.",
  ["console/cmd/lua/runtime_error"] = "Lua runtime error: %s",
  ["console/cmd/lua/syntax_error"] = "Lua syntax error: %s",
})

trx.console.register({
  name = "lua",
  help = "console/cmd/lua/help",
  args = function(parser)
    parser:rest("code")
  end,
  run = function(args)
    local err = trx.lua.eval_expr(args.code)
    if err == nil then
      return trx.console.Result.OK
    end
    if err.kind == "syntax" then
      return trx.console.Result.FAILURE,
        trx.locale.format("console/cmd/lua/syntax_error", err.message)
    end
    return trx.console.Result.FAILURE,
      trx.locale.format("console/cmd/lua/runtime_error", err.message)
  end,
})
