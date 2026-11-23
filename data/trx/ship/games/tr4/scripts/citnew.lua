local cutscenes = require("tr4.cutscenes")

local LEVEL_END_CUTSCENE = 15
local JEAN_YVES_CUTSCENE = 16
local FLAME = trx.catalog.objects.flame_emitter_tr4_wall
local AMBER_LIGHT = trx.catalog.objects.amber_light
local AMBER_LIGHT_ROOM = 177
local AMBER_LIGHT_FRAME = 2980

local function in_room(object, room_num)
  return trx.items.query:of_object(object):in_room(room_num):matches()
end

local function switch_on(object, room_num)
  for _, item in ipairs(in_room(object, room_num)) do
    item:trigger()
  end
end

local function switch_off(object, room_num)
  for _, item in ipairs(in_room(object, room_num)) do
    item:trigger({ type = trx.items.TriggerType.ANTITRIGGER })
    item:deactivate()
  end
end

cutscenes.register(JEAN_YVES_CUTSCENE, {
  chat = {
    {
      lara = true,
      ranges = {
        { 386, 419 },
        { 430, 489 },
        { 1021, 1058 },
        { 1072, 1175 },
      },
    },
    {
      actor = 3,
      node = 18,
      ranges = {
        { 97, 136 },
        { 174, 197 },
        { 214, 308 },
        { 329, 372 },
        { 511, 672 },
        { 684, 781 },
        { 812, 845 },
        { 863, 988 },
      },
      speech_heads = {
        trx.catalog.objects.actor_2_speech_head_1,
        trx.catalog.objects.actor_2_speech_head_2,
      },
    },
    {
      actor = 1,
      node = 21,
      ranges = {
        { 1614, 1641 },
        { 1657, 1726 },
        { 1741, 1760 },
        { 1775, 1974 },
      },
      speech_heads = {
        trx.catalog.objects.actor_1_speech_head_1,
        trx.catalog.objects.actor_1_speech_head_2,
      },
    },
  },
  hidden_actors = { 1, 2, 4, 5, 6, 7, 8, 9 },
  actor_cues = {
    [1194] = { hide = { 3 } },
    [1257] = { show = { 2 } },
    [1300] = { show = { 1, 9 } },
    [1374] = { hide = { 2 } },
    [1740] = { show = { 2 } },
    [2130] = { hide = { 2, 9 } },
    [2160] = { show = { 6 } },
    [2340] = { show = { 7 } },
    [2401] = { hide = { 1 } },
    [2501] = { show = { 4, 5, 8 }, hide = { 6, 7 } },
    [2820] = { show = { 1 } },
  },
  frames = {
    [AMBER_LIGHT_FRAME] = function()
      switch_on(AMBER_LIGHT, AMBER_LIGHT_ROOM)
    end,
  },
  on_start = function()
    trx.cutscenes.set_lara_return({ x = 61868, y = 0, z = 44994 }, -16060)
    switch_on(FLAME, 169)
    switch_on(FLAME, 184)
  end,
  on_end = function()
    switch_off(FLAME, 169)
    switch_off(FLAME, 184)
    switch_off(AMBER_LIGHT, AMBER_LIGHT_ROOM)
  end,
})

cutscenes.register(LEVEL_END_CUTSCENE, {
  chat = {
    {
      lara = true,
      ranges = {
        { 684, 715 },
        { 1314, 1372 },
        { 1392, 1417 },
        { 1792, 1824 },
      },
    },
    {
      actor = 1,
      node = 21,
      ranges = {
        { 290, 317 },
        { 330, 375 },
        { 388, 673 },
        { 729, 1301 },
      },
      speech_heads = {
        trx.catalog.objects.actor_1_speech_head_1,
        trx.catalog.objects.actor_1_speech_head_2,
      },
    },
  },
  hidden_actors = { 1, 2, 3, 4, 5, 6, 7, 8 },
  actor_cues = {
    [201] = { show = { 1 } },
    [360] = { show = { 2, 3, 4, 5 } },
    [681] = { hide = { 1, 2, 3, 4, 5 } },
    [781] = { show = { 1, 2, 3, 4, 5 } },
    [1306] = { hide = { 1, 2, 3, 4, 5 } },
    [1402] = { show = { 2, 3, 4, 5 } },
    [1552] = { hide = { 2, 3, 4, 5 } },
    [1601] = { show = { 6, 7 } },
    [1691] = { show = { 1, 2, 3, 4, 5, 8 }, hide = { 6 } },
    [1750] = { hide = { 1, 2, 3, 4, 5 } },
  },
  on_start = function()
    switch_on(FLAME, 184)
    switch_on(AMBER_LIGHT, AMBER_LIGHT_ROOM)
  end,
  on_end = trx.game.end_level,
})

trx.cutscenes[LEVEL_END_CUTSCENE]:on_end(function()
  trx.inventory.set_count(trx.catalog.objects.quest_item_1, 1)
end)
