require("common.overlay")
require("common.water_color").declare({
  order = { "pc", "custom" },
  modes = {
    pc = "80E0FF",
  },
})

require("tr3.quest_items").initialise()
require("common.save_crystal").initialise()
