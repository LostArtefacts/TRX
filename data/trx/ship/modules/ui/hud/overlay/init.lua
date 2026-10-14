-- The in-game overlay: the bars, the ammunition count, and the readouts that
-- say where Lara is and what she is doing.
--
-- The lines of text the rest of the engine asks for, the arrows beside them
-- and the version are not here. Those belong to whoever sets them and are
-- drawn from the engine, into the same regions these widgets sit in.
--
-- Every widget is made once, when this is loaded, and given signals rather
-- than values. A signal moving brings a widget up to date; a frame in which
-- nothing moves measures nothing and reads nothing.
--
-- Each part of the overlay is built in a module of its own, which holds the
-- signals only it reads. What several parts read is built once, here, and
-- handed to each.
--
-- Nothing here draws. The widgets own that, and the region a widget sits in
-- owns where it goes, so what this places is stacked beside what the
-- inventory ring and the dialogs place rather than over it.

local ammo = require("common.ui.hud.overlay.ammo")
local bars = require("common.ui.hud.overlay.bars")
local photo_mode = require("common.ui.hud.overlay.photo_mode")
local pickups = require("common.ui.hud.overlay.pickups")
local readouts = require("common.ui.hud.overlay.readouts")

local M = {}

-- What more than one part of the overlay reads.
---@class common.ui.hud.overlay.Shared
---@field ui_enabled trx.signal.Signal Whether the game interface is on.
---@field playing trx.signal.Signal Whether a level is running.
---@field armed trx.signal.Signal Whether Lara has her guns ready.

-- Builds the overlay and places it. Called once, from a game script.
function M.setup()
  -- The photo mode panel and the pickups belong to the overlay, so every
  -- game gets them with the rest of it.
  photo_mode.setup()
  pickups.setup()

  -- What more than one part reads. Everything here is a signal; a widget is
  -- given one rather than a value, so a frame in which none of them moved
  -- costs nothing.
  ---@type common.ui.hud.overlay.Shared
  local state = {
    ui_enabled = trx.signal.config("ui.enable_game_ui"),
    playing = trx.game.signals.is_playing,
    armed = trx.lara.signals.gun_status:eq(trx.lara.GunState.READY),
  }

  bars.setup(state)
  ammo.setup(state)
  readouts.setup(state)
end

return M
