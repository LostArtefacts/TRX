-- Once each henchman dies in the first area of the level, another will spawn
-- in. Items 5 and 6 are activated by regular triggers. The second guide is item
-- 13, so appears towards the end of the scene.
local henchman_map = {
  [5] = 7,
  [6] = 9,
  [7] = 8,
  [8] = 10,
  [9] = 11,
  [11] = 13,
}

local function henchman_killed(item)
  local next_enemy = henchman_map[item.num]
  if next_enemy ~= nil then
    trx.items[next_enemy]:trigger()
  end
end

trx.events.on_game_start(function(is_save)
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.EXTENDED
  trx.objects.guide.properties.guides_lara = false
  trx.objects.henchman_1.properties.seeks_equipment = false

  for henchman_idx, _ in pairs(henchman_map) do
    trx.items[henchman_idx]:on_kill(henchman_killed)
  end

  if not is_save then
    local guide = trx.items[13]
    guide.pos = { x = guide.pos.x, y = guide.pos.y + 128, z = guide.pos.z }
  end
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
