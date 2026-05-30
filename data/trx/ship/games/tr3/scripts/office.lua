trx.events.on_pickup(function(pickup_item)
  local item = trx.items[pickup_item + 1]
  if item.object_id == trx.catalog.objects.quest_item_3 then
    trx.rooms.flip_effect(trx.catalog.flip_effects.finish_level)
  end
end)
