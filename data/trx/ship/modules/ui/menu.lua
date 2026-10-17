-- Draws the game's menu with the menu system the player picks: the inventory
-- ring, or the flat menu of the original TR4. The flat menu leaves to the
-- ring what it does not draw. The quick save and load screens follow
-- the same pick: the passport's page, or the flat menu's list of slots.
--
-- Usage:
--   require("common.ui.menu").setup()

local flat_menu = require("common.ui.flat_menu")
local passport = require("common.ui.screens.passport")
local ring_screen = require("common.ui.ring.screen")

local M = {}

local SETTING = "ui.menu_system"

-- Written out in full because the scanner that carries these into the
-- shipped strings reads a key only where it is a literal.
trx.locale.declare({
  ["settings/ui.menu_system/title"] = "Menu system",
  ["settings/ui.menu_system/description"] = "How the game shows its menus. "
    .. "Ring shows what Lara carries on a ring that turns, as in TR1 to TR3. "
    .. "Flat shows it in a row across the screen with a list of what can be "
    .. "done with it, as in the original TR4.",
  ["settings/ui.menu_system/values/ring"] = "Ring",
  ["settings/ui.menu_system/values/flat"] = "Flat",
})

local function open(ctx)
  if trx.config.get(SETTING) == "flat" then
    local layer = flat_menu.open(ctx)
    if layer ~= nil then
      return layer
    end
  end
  return ring_screen.open(ctx)
end

local function open_save_load(ctx)
  if trx.config.get(SETTING) == "flat" then
    return flat_menu.open_save_load(ctx)
  end
  return passport.open_save_load(ctx)
end

-- Declares the setting and draws the menu. Called once, from a global script.
-- TR4 picks the flat menu by default, as the original draws its menus flat.
function M.setup()
  trx.config.declare({
    key = SETTING,
    kind = "dynamic_enum",
    values = { "ring", "flat" },
    default = trx.game.version == 4 and "flat" or "ring",
    ui = { tab = "graphic_ui", after = "ui.menu_style" },
  })
  ring_screen.setup()
  flat_menu.setup()
  trx.ui.screens.define(trx.ui.Screen.MENU, open)
  trx.ui.screens.define(trx.ui.Screen.SAVE_LOAD, open_save_load)
end

return M
