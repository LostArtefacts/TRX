local cutscenes = require("tr4.cutscenes")

local AMULET_CUTSCENE = 14
local AMULET_PULL_FRAME = 393
local AMULET_STORE_FRAME = 486
local SETH_IDLE_BITS = 0x5
local SETH_LIVE_BITS = 0x3

local seth_item = nil

local function initialise_seth(is_save)
  seth_item =
    trx.items.query:of_object(trx.catalog.objects.seth_sarcophagus):first()
  if seth_item:is_valid() and not is_save then
    seth_item.mesh_bits = SETH_IDLE_BITS
  end
end

local function pull_amulet()
  if seth_item:is_valid() then
    seth_item.mesh_bits = SETH_LIVE_BITS
  end
end

local function store_amulet()
  if not trx.inventory.has(trx.catalog.objects.quest_item_1) then
    trx.inventory.give(trx.catalog.objects.quest_item_1)
    trx.stats.pickups.count = trx.stats.pickups.count + 1
  end
end

local function end_amulet_scene()
  pull_amulet()
  store_amulet()
  trx.rooms.flip(1)
end

cutscenes.register(AMULET_CUTSCENE, {
  frames = {
    [AMULET_PULL_FRAME] = pull_amulet,
    [AMULET_STORE_FRAME] = store_amulet,
  },
  on_end = end_amulet_scene,
})

trx.events.on_cutscene_start(function(cutscene)
  if cutscene.num == AMULET_CUTSCENE then
    trx.cutscenes.set_lara_return({ x = 27282, y = 256, z = 65654 }, 0)
  end
end)

trx.events.on_game_start(function(is_save)
  trx.items[3].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[135].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW

  trx.objects.animating_13.properties.collidable = false
  trx.objects.animating_14.properties.collidable = false

  trx.objects.waterfall_1.properties.loop_sound = trx.items.WaterfallSound.SAND
  trx.objects.waterfall_1.properties.hide_when_inactive = true

  trx.objects.scaled_spikes.properties.orientation = 0
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.EXTENDED
  trx.items[22].properties.orientation = 4
  trx.items[22].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[23].properties.orientation = 2
  trx.items[23].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[24].properties.orientation = 6
  trx.items[24].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[25].properties.orientation = 7
  trx.items[25].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[26].properties.orientation = 5
  trx.items[26].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[27].properties.orientation = 1
  trx.items[27].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[28].properties.orientation = 3
  trx.items[28].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[29].properties.orientation = 11
  trx.items[29].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[30].properties.orientation = 15
  trx.items[30].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[31].properties.orientation = 13
  trx.items[31].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[33].properties.orientation = 8
  trx.items[33].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[40].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[41].properties.orientation = 6
  trx.items[41].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[42].properties.orientation = 2
  trx.items[42].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[43].properties.orientation = 1
  trx.items[43].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[44].properties.orientation = 7
  trx.items[44].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[45].properties.orientation = 3
  trx.items[45].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[46].properties.orientation = 5
  trx.items[46].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[47].properties.orientation = 4
  trx.items[47].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[52].properties.orientation = 3
  trx.items[53].properties.orientation = 3
  trx.items[54].properties.orientation = 5
  trx.items[55].properties.orientation = 5
  trx.items[56].properties.orientation = 5
  trx.items[57].properties.orientation = 5
  trx.items[59].properties.orientation = 3
  trx.items[60].properties.orientation = 3
  trx.items[61].properties.orientation = 3
  trx.items[62].properties.orientation = 3
  trx.items[63].properties.orientation = 3
  trx.items[64].properties.orientation = 3
  trx.items[65].properties.orientation = 3
  trx.items[66].properties.orientation = 3
  trx.items[67].properties.orientation = 5
  trx.items[68].properties.orientation = 5
  trx.items[69].properties.orientation = 5
  trx.items[70].properties.orientation = 5
  trx.items[76].properties.orientation = 4
  trx.items[77].properties.orientation = 6
  trx.items[80].properties.orientation = 8
  trx.items[80].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[81].properties.orientation = 10
  trx.items[81].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[82].properties.orientation = 14
  trx.items[82].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[83].properties.orientation = 9
  trx.items[83].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[84].properties.orientation = 11
  trx.items[84].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[85].properties.orientation = 15
  trx.items[85].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[86].properties.orientation = 13
  trx.items[86].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[87].properties.orientation = 12
  trx.items[87].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.LOOPING
  trx.items[91].properties.orientation = 7
  trx.items[92].properties.orientation = 7
  trx.items[93].properties.orientation = 1
  trx.items[94].properties.orientation = 1
  trx.items[95].properties.orientation = 1
  trx.items[96].properties.orientation = 1
  trx.items[98].properties.orientation = 4
  trx.items[109].properties.orientation = 4
  trx.items[110].properties.orientation = 4
  trx.items[111].properties.orientation = 4
  trx.items[112].properties.orientation = 4
  trx.items[116].properties.orientation = 4

  trx.items[214].properties.use_idle_pose = true
  trx.items[215].properties.use_idle_pose = true

  for i = 183, 186 do
    trx.items[i].properties.speed = 7
    trx.items[i].properties.travel_distance = 20
  end
  for i = 226, 229 do
    trx.items[i].properties.speed = 3
    trx.items[i].properties.travel_distance = 32
  end

  initialise_seth(is_save)
end)

require("tr4.inv_setup").apply({
  puzzle_item_1 = { scale = 1024, rot_x = -90, rot_y = 180 },
  puzzle_item_2 = { scale = 1024, rot_x = -90, rot_y = 180 },
  puzzle_item_3 = { scale = 1024, rot_y = 180 },
  puzzle_item_4 = { scale = 1280, offset_y = 5, rot_y = 180 },
})
