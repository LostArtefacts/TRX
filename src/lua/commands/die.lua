-- Kills Lara with an explosion. An easter-egg command, so it carries no help.

local function run()
  if not trx.game.is_playable then
    return trx.console.Result.UNAVAILABLE
  end

  local lara = trx.lara.item
  if lara == nil or lara.hit_points <= 0 then
    return trx.console.Result.UNAVAILABLE
  end

  trx.sound.play(trx.catalog.samples.LARA_FALL, { pos = lara.pos })
  trx.sound.play(trx.catalog.samples.EXPLOSION_1, { pos = lara.pos })
  lara:shatter(1)
  lara.hit_points = 0
  lara.is_one_shot = true
  return trx.console.Result.OK
end

trx.console.register({
  name = "abortion",
  aliases = { "natlasucks", "natla-sucks", "natlastinks", "natla-stinks" },
  run = run,
})
