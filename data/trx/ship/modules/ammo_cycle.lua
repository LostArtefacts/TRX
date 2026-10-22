-- Moves the weapon in Lara's hands on to its next kind of ammunition when the
-- player presses the cycle ammo key.

local ammo = require("common.ui.ammo")

local M = {}

-- Listens for the key. Called once, as the shipped scripts load.
function M.setup()
  local lara = trx.lara
  trx.input.signals.pressed(trx.input.Role.CYCLE_AMMO):on(function(pressed)
    if
      not pressed
      or not trx.game.signals.is_playing:get()
      or lara.signals.gun_status:get() ~= lara.GunState.READY
    then
      return
    end
    local weapon = trx.weapons[lara.equipped_gun]
    if weapon == nil then
      return
    end
    local kind = ammo.next_kind(weapon)
    if kind ~= nil then
      ammo.choose(weapon, kind)
    end
  end)
end

return M
