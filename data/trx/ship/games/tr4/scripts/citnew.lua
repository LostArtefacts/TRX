local cutscenes = require("tr4.cutscenes")

local LEVEL_END_CUTSCENE = 15
local JEAN_YVES_CUTSCENE = 16

cutscenes.register(JEAN_YVES_CUTSCENE, {
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
  on_start = function()
    trx.cutscenes.set_lara_return({ x = 61868, y = 0, z = 44994 }, -16060)
  end,
})

cutscenes.register(LEVEL_END_CUTSCENE, {
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
  on_end = trx.game.end_level,
})

trx.cutscenes[LEVEL_END_CUTSCENE]:on_end(function()
  trx.inventory.set_count(trx.catalog.objects.quest_item_1, 1)
end)
