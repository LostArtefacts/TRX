trx.events.on_game_start(function(level)
  local query = trx.items.query:of_object(trx.catalog.objects.animating_ext_1)
  for _, item in ipairs(query:matches()) do
    item.properties.kill_on_trigger = true
  end
end)
