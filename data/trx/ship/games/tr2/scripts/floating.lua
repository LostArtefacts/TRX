trx.events.on_game_start(function()
  -- The level gives its second and third secret each other's sprite, so with
  -- 3D pickups off the wrong dragon lies in each place. The models are right,
  -- it's the sprites that are wrongly ordered.
  trx.objects.swap_sprite(
    trx.catalog.objects.secret_2,
    trx.catalog.objects.secret_3
  )
end)
