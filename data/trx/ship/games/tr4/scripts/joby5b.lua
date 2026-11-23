local cutscenes = require("tr4.cutscenes")

local LEVEL_END_CUTSCENE = 23

-- The statue changes as the scene plays: on each of these frames, it hides
-- the meshes in `hide` and shows the meshes in `show`.
local STATUE_CHANGES = {
  [194] = { hide = 0x200, show = 0x20 },
  [251] = { hide = 0x400, show = 0x10 },
  [350] = { hide = 0x40, show = 0x08 },
  [440] = { hide = 0x04, show = 0x80 },
  [514] = { hide = 0x02, show = 0x100 },
  [556] = { hide = 0, show = 0x800 },
}

local function change_statue(change)
  local statues =
    trx.items.query:of_object(trx.catalog.objects.horus_statue):matches()
  for _, statue in ipairs(statues) do
    statue.mesh_bits = (statue.mesh_bits & ~change.hide) | change.show
  end
end

local statue_frames = {}
for frame, change in pairs(STATUE_CHANGES) do
  statue_frames[frame] = function()
    change_statue(change)
  end
end

trx.events.on_game_start(function()
  trx.objects.animating_16.properties.collidable = false
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.EXTENDED
  trx.items[102].properties.orientation = 6
  trx.items[102].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[103].properties.orientation = 2
  trx.items[103].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[104].properties.orientation = 14
  trx.items[104].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[105].properties.orientation = 10
  trx.items[105].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[114].properties.orientation = 6
  trx.items[114].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[115].properties.orientation = 2
  trx.items[115].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[116].properties.orientation = 14
  trx.items[116].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[117].properties.orientation = 10
  trx.items[117].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
end)

require("tr4.inv_setup").apply({
  key_item_11 = { scale = 1200, offset_y = 5, rot_x = -90 },
  key_item_12 = { scale = 768, offset_y = 4, rot_x = -90 },
  puzzle_item_5 = { scale = 1200, offset_y = 23, rot_y = 90 },
})

cutscenes.register(LEVEL_END_CUTSCENE, {
  frames = statue_frames,
  on_end = trx.game.end_level,
})

trx.cutscenes[LEVEL_END_CUTSCENE]:on_end(function()
  for i = 2, 6 do
    trx.inventory.set_count(trx.catalog.objects["quest_item_" .. i], 0)
  end
end)
