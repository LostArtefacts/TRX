trx.events.on_game_start(function()
  local quest_objects = {
    trx.objects.quest_item_1,
    trx.objects.quest_item_2,
    trx.objects.quest_item_3,
    trx.objects.quest_item_4,
  }
  for _, obj in pairs(quest_objects) do
    obj.properties.glow_color = "#00F87C"
    obj.properties.rotation = trx.math.DEG_90 // 16
    obj.properties.show_pickup_aid = false
  end
end)
