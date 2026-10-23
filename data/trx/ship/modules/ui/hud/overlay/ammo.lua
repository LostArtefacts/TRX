-- The ammunition count of the in-game overlay.

local ammo = require("common.ui.ammo")
local place = require("common.ui.hud.overlay.place")

trx.locale.declare({
  ["general/overlay/item_count_fmt_pc"] = "\\{small}%s",
  ["general/overlay/item_count_fmt_ps1"] = "\\{small}%s",
  ["settings/ui.ammo_counter_location/title"] = "Ammo counter location",
  ["settings/ui.ammo_counter_location/description"] = "Location where the ammo counter is displayed.",
})

local M = {}

trx.config.declare({
  key = "ui.ammo_counter_location",
  kind = "dynamic_enum",
  values = place.LOCATIONS,
  default = "top_right",
})
place.normalize("ui.ammo_counter_location")

-- Builds the count and places it. `state` holds what the rest of the overlay
-- reads too.
---@param state common.ui.hud.overlay.Shared
function M.setup(state)
  local ui = trx.ui
  local signal = trx.signal
  local lara = trx.lara

  -------------------------------------------------------------------------------
  -- how many shots she has left for what she is holding
  -------------------------------------------------------------------------------

  do
    -- The weapon the count answers for: the one the vehicle carries where she is
    -- riding an armed one, and otherwise the one in her hands.
    local function counted_gun()
      local vehicle_gun = lara.vehicle_gun
      if vehicle_gun ~= nil then
        return vehicle_gun, true
      end
      if not state.armed:get() then
        return nil
      end
      return lara.equipped_gun, false
    end

    local shown = state.ui_enabled & state.playing

    -- How long the count shows the name of a kind after the player cycles
    -- the weapon in her hands to it.
    local CAPTION_TICKS = trx.game.LOGIC_FPS * 2
    local last_gun = nil
    local caption_ticks = 0
    trx.events.on_level_unload(function()
      last_gun = nil
      caption_ticks = 0
    end)
    ammo.cycled:on(function(cycle)
      if cycle ~= nil then
        caption_ticks = CAPTION_TICKS
      end
    end)

    -- What she is carrying is not a signal, so the count is read once a tick,
    -- and only while the interface it is drawn on is up.
    local text = signal.polled_while(shown, function()
      local gun, from_vehicle = counted_gun()
      if gun == nil then
        caption_ticks = 0
        return nil
      end
      local weapon = trx.weapons[gun]
      if weapon == nil or weapon.has_infinite_ammo then
        return nil
      end
      if not from_vehicle and weapon.ammo_object == nil then
        return nil
      end

      local kind = not from_vehicle and trx.inventory.ammo_kind(gun) or nil
      if gun ~= last_gun then
        caption_ticks = 0
      end
      last_gun = gun

      local inner
      if caption_ticks > 0 and kind ~= nil then
        caption_ticks = caption_ticks - 1
        inner = ammo.caption(weapon, kind)
      else
        local shots = trx.inventory.shots(gun)
        local icon = trx.game.tr_version == 1 and weapon.ammo_icon or nil
        inner = icon ~= nil and string.format("%6d %s", shots, icon)
          or string.format("%6d", shots)
      end
      -- Both keys are named where they are used, so what carries them into the
      -- shipped strings can see them.
      if trx.config.get("ui.menu_style") == "ps1" then
        return trx.locale.format("general/overlay/item_count_fmt_ps1", inner)
      end
      return trx.locale.format("general/overlay/item_count_fmt_pc", inner)
    end)

    place.at_setting(
      "ui.ammo_counter_location",
      ui.widgets.Label({
        text = text:map(function(value)
          return value or ""
        end),
        scale = 1.5,
        visible = shown & text:map(function(value)
          return value ~= nil
        end),
      })
    )
  end
end

return M
