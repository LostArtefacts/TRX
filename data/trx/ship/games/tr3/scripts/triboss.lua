trx.events.on_pickup(function(pickup_item)
  local item = trx.items[pickup_item]
  if item.object_id == trx.catalog.objects.quest_item_4 then
    trx.game.end_level()
  end
end)

trx.events.on_game_start(function()
  local props = trx.objects.quest_item_4.properties
  props.glow_color = "#00F87C"
  props.rotation = trx.math.DEG_90 // 16
  props.show_pickup_aid = false
end)
