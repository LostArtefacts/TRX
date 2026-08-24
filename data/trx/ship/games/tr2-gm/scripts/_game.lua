require("common.overlay")
require("common.water_color").declare({
  order = { "pc_hardware", "pc_software", "custom" },
  modes = {
    pc_hardware = "80E0FF",
    pc_software = "AAAAFF",
  },
})

require("common.save_crystal").initialise()
