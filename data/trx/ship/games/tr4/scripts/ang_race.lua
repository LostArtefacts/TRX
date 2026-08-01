trx.events.on_game_start(function(is_save)
  trx.objects.animating_14.properties.collidable = false
  trx.objects.animating_15.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.lara.holsters_visible = trx.lara.has_pistol_weapon
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.HIDDEN_REACH
end)
