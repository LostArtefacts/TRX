trx.events.on_game_start(function()
  local props = trx.objects.strobe_light.properties
  props.requires_alarm_active = true
  trx.creatures.add_ally(trx.catalog.objects.prisoner)
end)

trx.events.on_pickup(function(pickup_item)
  local item = trx.items[pickup_item]
  if item.object_id == trx.catalog.objects.quest_item_2 then
    trx.rooms.flip_effect(trx.catalog.flip_effects.finish_level)
  end
end)
