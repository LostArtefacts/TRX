-- Plays a demo.
--
-- Usages:
--   /demo        the next demo in rotation
--   /demo 2      a demo by number

local function run(args)
  local demos = trx.game.demos
  if #demos == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/play_demo/none")
  end

  local demo
  if args == "" then
    demo = trx.game.play_demo()
  else
    local num = tonumber(args)
    if num == nil or num % 1 ~= 0 then
      return trx.console.Result.BAD_INVOCATION
    end
    if num < 1 or num > #demos then
      return trx.console.Result.FAILURE,
        trx.locale.get("console/cmd/play_demo/invalid")
    end
    demo = trx.game.play_demo(num)
  end

  return trx.console.Result.OK,
    trx.locale.format("console/cmd/play_demo/loading", demo.num)
end

trx.console.register({
  name = "demo",
  help = "console/cmd/play_demo/help",
  run = run,
})
