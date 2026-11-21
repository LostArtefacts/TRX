trx.events.on_game_start(function(is_save)
  trx.items[4].properties.crowbar = true
  trx.items[57].properties.crowbar = true

  if not is_save then
    -- Fix enemy 41 clipping down to the track when activated.
    local enemy = trx.items[41]
    enemy.pos = { x = enemy.pos.x, y = -1536, z = enemy.pos.z }
  end
end)
