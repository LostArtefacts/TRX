trx.events.on_game_start(function()
  trx.rules.set("music.accumulate_trigger_masks", true)
end)

require("common.water_color").declare({
  order = { "tombati", "dos", "custom" },
  modes = {
    tombati = "72FFFF",
    dos = "99B2FF",
  },
})
