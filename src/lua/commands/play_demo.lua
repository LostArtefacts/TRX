-- Plays a demo.
--
-- Usages:
--   /demo        the next demo in rotation
--   /demo 2      a demo by number

trx.locale.declare({
  ["console/cmd/play_demo/help"] = "Plays a demo with the given number.",
  ["console/cmd/play_demo/invalid"] = "Invalid demo",
  ["console/cmd/play_demo/loading"] = "Loading demo %d",
  ["console/cmd/play_demo/none"] = "This game has no demos",
})

local function run(args)
  local demos = trx.game.demos
  if #demos == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/play_demo/none")
  end

  local demo
  if args.num == nil then
    demo = trx.game.play_demo()
  else
    if args.num < 1 or args.num > #demos then
      return trx.console.Result.FAILURE,
        trx.locale.get("console/cmd/play_demo/invalid")
    end
    demo = trx.game.play_demo(args.num)
  end

  return trx.console.Result.OK,
    trx.locale.format("console/cmd/play_demo/loading", demo.num)
end

trx.console.register({
  name = "demo",
  help = "console/cmd/play_demo/help",
  args = function(parser)
    parser:positional("num", { type = "integer", optional = true })
  end,
  run = run,
})
