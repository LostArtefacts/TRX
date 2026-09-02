local function fix_fan_setup()
  -- Slow down the fans in room 77 before Lara re-enters the water, but only
  -- if she has completed the required puzzle.
  local fan_pad = trx.zones.tile({ x = 28160, y = -8192, z = 53760 })
  fan_pad:on_enter(function()
    if
      trx.rooms.flipped
      or trx.items[94].is_one_shot
      or trx.items[96].trigger_mask ~= 31
    then
      return
    end

    trx.items[94]:trigger({ one_shot = true })
    trx.items[95]:trigger({ one_shot = true })
  end)
end

trx.events.on_game_start(function()
  trx.objects.propeller_2.properties.damage = 1000
  trx.objects.propeller_3.properties.damage = 1000

  if trx.config.get("gameplay.fix_floor_data_issues") then
    fix_fan_setup()
  end
end)
