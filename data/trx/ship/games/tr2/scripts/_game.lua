require("common.overlay")
require("common.water_color").declare({
  order = { "pc_hardware", "pc_software", "ps1", "custom" },
  modes = {
    pc_hardware = "80E0FF",
    pc_software = "AAAAFF",
    -- stylua: ignore
    ps1 = {
      assault  = "80FFFF", -- Lara's Home
      wall     = "B2E5E5", -- The Great Wall
      boat     = "CCFF80", -- Venice
      venice   = "CCFF80", -- Bartoli's Hideout
      opera    = "CCFF80", -- Opera House
      rig      = "80FFFF", -- Offshore Rig
      platform = "80FFFF", -- Diving Area
      unwater  = "80FFFF", -- 40 Fathoms
      keel     = "80FFFF", -- Wreck of the Maria Doria
      living   = "80FFFF", -- Living Quarters
      deck     = "80FFFF", -- The Deck
      skidoo   = "B2E5E5", -- Tibetan Foothills
      monastry = "80FFFF", -- Barkhang Monastery
      catacomb = "80FFFF", -- Catacombs of the Talion
      icecave  = "80FFFF", -- Ice Palace
      emprtomb = "CCFF99", -- Temple of Xian
      floating = "CCFFCC", -- Floating Islands
      xian     = "CCFFCC", -- Dragon's Lair
      house    = "80FFFF", -- Home Sweet Home
    },
  },
})

require("common.save_crystal").initialise()
