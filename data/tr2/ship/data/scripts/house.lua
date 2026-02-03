local dagger_hips = 7

trx.events.on_game_start(function(level, is_save)
  -- TODO: remove in TRX 1.5.
  trx.lara.holsters_visible = trx.lara.has_pistol_weapon
  if trx.lara.extra_anim == -1 then
    trx.lara.set_extra_equipment(0, dagger_hips)
  end
end)
