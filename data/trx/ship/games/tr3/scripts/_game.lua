require("common.overlay")
require("common.water_color").declare({
  order = { "pc", "ps1", "custom" },
  modes = {
    pc = "80E0FF",
    -- The two levels with no water carry the PC color, so the mode still has
    -- an answer for them.
    -- stylua: ignore
    ps1 = {
      house    = "CCFF80", -- Lara's Home
      jungle   = "CCFF80", -- Jungle
      temple   = "CCFF80", -- Temple Ruins
      quadchas = "CCFF80", -- The River Ganges
      tonyboss = "CCFF80", -- Caves of Kaliya
      shore    = "80FFFF", -- Coastal Village
      crash    = "FFFFFF", -- Crash Site
      rapids   = "FFFFFF", -- Madubu Gorge
      triboss  = "80E0FF", -- Temple of Puna
      roofs    = "FFFFFF", -- Thames Wharf
      sewer    = "CCFF80", -- Aldwych
      tower    = "CCFF80", -- Lud's Gate
      office   = "CCFF80", -- City
      nevada   = "FFFFFF", -- Nevada Desert
      compound = "FFFFFF", -- High Security Compound
      area51   = "FFFFFF", -- Area 51
      antarc   = "80FFFF", -- Antarctica
      mines    = "CCFFCC", -- RX-Tech Mines
      city     = "80E0FF", -- Lost City of Tinnos
      chamber  = "80E0FF", -- Meteorite Cavern
      stpaul   = "B2E6E6", -- All Hallows
    },
  },
})

require("tr3.quest_items").initialise()
require("common.save_crystal").initialise()
