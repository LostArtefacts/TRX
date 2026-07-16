-- Kills Lara with an explosion. An easter-egg command, so it carries no help.

-- The catalog names a sample the same way across games; the level's own slot for
-- it differs, so it is looked up before playing. A game without the sample skips
-- it rather than erroring.
local function play_sample(sfx, pos)
  local slot = trx.catalog.to_slot(trx.catalog.Context.SAMPLES, sfx)
  if slot ~= nil and trx.sound.samples[slot] ~= nil then
    trx.sound.play(slot, { pos = pos })
  end
end

local function run(args)
  if args ~= "" then
    return trx.console.Result.BAD_INVOCATION
  end

  if not trx.game.is_playable then
    return trx.console.Result.UNAVAILABLE
  end

  local lara = trx.lara.item
  if lara == nil or lara.hit_points <= 0 then
    return trx.console.Result.UNAVAILABLE
  end

  play_sample(trx.catalog.samples.LARA_FALL, lara.pos)
  play_sample(trx.catalog.samples.EXPLOSION_1, lara.pos)
  lara:shatter(1)
  lara.hit_points = 0
  lara.is_one_shot = true
  return trx.console.Result.OK
end

-- The regex the C command matched spelled out.
trx.console.register({
  name = "abortion",
  aliases = { "natlasucks", "natla-sucks", "natlastinks", "natla-stinks" },
  run = run,
})
