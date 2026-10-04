local function start_lara_on_jeep()
  local jeep = trx.items[0]
  jeep.pos = trx.lara.item.pos
  jeep.rot = { x = 0, y = trx.lara.item.rot.y, z = 0 }
  trx.waypoints.current = 1
end

trx.events.on_game_start(function(is_save)
  trx.items[6].properties.requires_heavy_trigger = true
  trx.items[8].properties.requires_heavy_trigger = true
  trx.items[9].properties.requires_heavy_trigger = true
  trx.items[68].properties.requires_heavy_trigger = true
  trx.items[69].properties.requires_heavy_trigger = true
  trx.items[71].properties.requires_heavy_trigger = true
  trx.items[89].properties.requires_heavy_trigger = true
  trx.items[91].properties.requires_heavy_trigger = true
  trx.items[93].properties.requires_heavy_trigger = true
  trx.items[95].properties.requires_heavy_trigger = true
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.EXTENDED

  if not is_save then
    start_lara_on_jeep()
  end

  trx.events.on_room_change(function(item, old_room_num, new_room_num)
    -- Prevent Lara's jeep closing gates 77 and 78 ahead of the other jeep.
    if item.object_id == trx.catalog.objects.jeep then
      trx.objects.jeep.properties.is_heavy = new_room_num ~= 52
        and new_room_num ~= 68
    end
  end)
end)

require("tr4.inv_setup").apply({
  puzzle_item_1 = {
    scale = 1024,
    offset_y = 8,
    rot_x = 67.5,
    rot_y = 45,
    rot_z = 90,
  },
})
