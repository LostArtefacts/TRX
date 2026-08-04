trx.events.on_game_start(function()
  local props = trx.objects.quest_item_1.properties
  props.glow_color = "#00F87C"
  props.rotation = trx.math.DEG_90 // 16
  props.show_pickup_aid = false
end)
