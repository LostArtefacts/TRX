-- Evaluates a string of Lua code.
--
-- Usages:
--   /lua trx.lara.item.hit_points = 100

trx.console.register({
  name = "lua",
  help = "console/cmd/lua/help",
  run = function(args)
    if args == "" then
      return trx.console.Result.BAD_INVOCATION
    end
    local err = trx.lua.eval_expr(args)
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
