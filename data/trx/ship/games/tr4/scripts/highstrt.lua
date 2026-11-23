local cutscenes = require("tr4.cutscenes")

local TROOPS_CUTSCENES = { 25, 26 }
local LEVEL_END_CUTSCENE = 11

-- Takes every running enemy out of the level, as the original game does when
-- this scene starts, so that none attacks Lara while it plays.
local function remove_enemies()
  for _, item in ipairs(trx.items.query:simulated():matches()) do
    if trx.objects[item.object_id].is_intelligent then
      item.is_visible = false
      item:deactivate()
    end
  end
end

trx.events.on_game_start(function()
  trx.objects.animating_13.properties.collidable = false
end)

require("tr4.inv_setup").apply({
  puzzle_item_1 = { scale = 768, offset_y = 4 },
  puzzle_item_2 = { scale = 1024, rot_x = 180, rot_y = 90, rot_z = 90 },
  puzzle_item_4 = { scale = 768, rot_x = -67.5, rot_y = 90, rot_z = -112.5 },
  puzzle_item_5 = { scale = 512, rot_x = -90, rot_y = 180 },
  puzzle_item_8 = { scale = 1024, offset_y = 8, rot_y = 180 },
  puzzle_item_1_combo_1 = { scale = 768, offset_y = 4, draws_at_pivot = true },
  puzzle_item_1_combo_2 = { scale = 768, offset_y = 2 },
  puzzle_item_2_combo_1 = { scale = 1024, rot_x = 180, rot_y = 90, rot_z = 90 },
  puzzle_item_2_combo_2 = { scale = 768, rot_x = 180, rot_y = 90, rot_z = 90 },
  puzzle_item_8_combo_1 = { scale = 1024, offset_y = 8, rot_y = 180 },
  puzzle_item_8_combo_2 = {
    scale = 640,
    offset_y = 4,
    rot_x = -90,
    rot_y = 180,
  },
})

local TROOP_SPEECH_HEADS = {
  trx.catalog.objects.actor_1_speech_head_1,
  trx.catalog.objects.actor_1_speech_head_2,
}

local TROOPS_CHAT = {
  [25] = {
    {
      lara = true,
      ranges = {
        { 476, 578 },
        { 976, 1018 },
        { 1028, 1069 },
        { 1317, 1340 },
        { 2065, 2178 },
        { 2835, 2865 },
      },
    },
    {
      actor = 1,
      node = 14,
      ranges = {
        { 289, 363 },
        { 378, 449 },
        { 596, 604 },
        { 635, 744 },
        { 764, 780 },
        { 811, 959 },
        { 1088, 1157 },
        { 1191, 1313 },
        { 1360, 2046 },
        { 2227, 2295 },
        { 2330, 2821 },
        { 2877, 3322 },
      },
      speech_heads = TROOP_SPEECH_HEADS,
    },
  },
  [26] = {
    {
      actor = 1,
      node = 14,
      ranges = {
        { 108, 432 },
      },
      speech_heads = TROOP_SPEECH_HEADS,
    },
  },
}

for _, num in ipairs(TROOPS_CUTSCENES) do
  cutscenes.register(num, {
    hide = { trx.catalog.objects.animating_5 },
    chat = TROOPS_CHAT[num],
    on_start = function()
      trx.cutscenes.set_lara_return({ x = 55396, y = -1792, z = 77122 }, -7105)
    end,
  })
end

cutscenes.register(LEVEL_END_CUTSCENE, {
  chat = {
    {
      lara = true,
      ranges = {
        { 1216, 1232 },
        { 1241, 1323 },
        { 1337, 1393 },
        { 1678, 1706 },
        { 1927, 1954 },
      },
    },
    {
      actor = 2,
      node = 14,
      ranges = {
        { 1141, 1181 },
        { 1509, 1666 },
        { 1758, 1914 },
        { 1964, 2016 },
        { 2194, 2261 },
      },
      speech_heads = {
        trx.catalog.objects.actor_1_speech_head_1,
        trx.catalog.objects.actor_1_speech_head_2,
      },
    },
  },
  hide = {
    trx.catalog.objects.animating_1,
    trx.catalog.objects.animating_5,
    trx.catalog.objects.motorbike,
  },
  hidden_actors = { 3, 4, 5, 6 },
  actor_cues = {
    [581] = { show = { 3 }, hide = { 1 } },
    [671] = { show = { 5 } },
    [694] = { hide = { 3 } },
    [763] = { show = { 1 } },
    [1030] = { hide = { 1 } },
    [1201] = { show = { 1 } },
    [1394] = { hide = { 1 } },
    [1423] = { show = { 3 } },
    [1460] = { hide = { 3 } },
    [1668] = { show = { 1 } },
    [1708] = { hide = { 1 } },
    [1950] = { show = { 3 } },
    [2251] = { hide = { 3 } },
    [2280] = { hide = { 2 } },
    [2383] = { show = { 1 } },
    [2790] = { show = { 4, 6 } },
  },
  on_start = remove_enemies,
  on_end = trx.game.end_level,
})
