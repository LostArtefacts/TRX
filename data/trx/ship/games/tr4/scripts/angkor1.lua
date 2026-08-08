trx.events.on_game_start(function(is_save)
  trx.objects.animating_13.properties.collidable = false
  trx.objects.animating_14.properties.collidable = false
  trx.objects.animating_15.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.lara.holsters_visible = trx.lara.has_pistol_weapon
end)

local BACKPACK_CUTSCENE = 5

trx.events.on_cutscene_start(function(cutscene_num)
  if cutscene_num == BACKPACK_CUTSCENE then
    -- The scene starts while Lara is crawling, and the crawlspace she triggers
    -- it from is no place to stand up in. The original engine carries her out
    -- to the floor beyond it.
    trx.cutscenes.set_lara_return({ x = 100938, y = 768, z = 58040 }, -32552)
  end
end)
