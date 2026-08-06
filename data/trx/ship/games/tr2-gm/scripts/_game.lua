trx.events.on_game_start(function()
  trx.rules.set("music.supports_delay", true)
end)

require("common.water_color").declare({
  order = { "pc_hardware", "pc_software", "custom" },
  modes = {
    pc_hardware = "80E0FF",
    pc_software = "AAAAFF",
  },
})
