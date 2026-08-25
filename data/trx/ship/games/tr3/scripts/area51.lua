trx.events.on_game_start(function()
  local props = trx.objects.strobe_light.properties
  props.requires_alarm_active = true
  trx.creatures.add_ally(trx.catalog.objects.prisoner)

  props = trx.objects.quest_item_2.properties
  props.glow_color = "#00F87C"
  props.rotation = trx.math.DEG_90 // 16
  props.show_pickup_aid = false

  local element115 = trx.items[44]
  if element115.trigger_mask == 0 then
    element115:deactivate()
  end
end)

trx.events.on_pickup(function(pickup_item)
  local item = trx.items[pickup_item]
  if item.object_id == trx.catalog.objects.quest_item_2 then
    trx.game.end_level()
  end
end)
