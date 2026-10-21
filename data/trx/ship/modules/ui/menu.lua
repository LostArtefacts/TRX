-- Draws the game's menu with the menu system the player picks: the inventory
-- ring, or the flat menu of the original TR4. The flat menu leaves to the
-- ring what it does not draw. The quick save and load screens follow
-- the same pick: the passport's page, or the flat menu's list of slots.
-- The flat menu system draws no frames anywhere, as the original draws none.
-- The pause screen follows the pick too, as the flat one leads to the
-- statistics and the settings, which the ring holds otherwise.
-- A change of the pick waits until no menu is open, so that a menu keeps the
-- system it opened with, as a change of the inventory background does.
--
-- Usage:
--   require("common.ui.menu").setup()

local flat_menu = require("common.ui.flat_menu")
local flat_pause = require("common.ui.flat_menu.pause")
local passport = require("common.ui.screens.passport")
local pause = require("common.ui.screens.pause")
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

-- The system the menus draw with, as last applied, and how many menus are
-- open.
local system = nil
local open_count = 0

-- Applies the picked system, unless a menu is open.
local function follow()
  if open_count > 0 then
    return
  end
  system = trx.config.get(SETTING)
  trx.ui.frames_shown = system ~= "flat"
end

-- Applies the picked system as a menu opens, and again once it closes.
-- Returns whether the menu draws with the flat system.
local function begin(ctx)
  follow()
  open_count = open_count + 1
  ctx:on_end(function()
    open_count = open_count - 1
    follow()
  end)
  return system == "flat"
end

local function open(ctx)
  if begin(ctx) then
    local layer = flat_menu.open(ctx)
    if layer ~= nil then
      return layer
    end
  end
  return ring_screen.open(ctx)
end

local function open_pause(ctx)
  if begin(ctx) then
    return flat_pause.open(ctx)
  end
  return pause.open(ctx)
end

local function open_save_load(ctx)
  if begin(ctx) then
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
  follow()
  trx.signal.config(SETTING):on(follow)
  ring_screen.setup()
  trx.ui.screens.define(trx.ui.Screen.MENU, open)
  trx.ui.screens.define(trx.ui.Screen.SAVE_LOAD, open_save_load)
  trx.ui.screens.define(trx.ui.Screen.PAUSE, open_pause)
end

return M
