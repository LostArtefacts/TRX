trx.events.on_game_start(function(is_save)
  trx.lara.holsters_visible = trx.lara.has_pistol_weapon
  if trx.lara.extra_anim == -1 then
    trx.lara.set_extra_equipment(
      trx.lara.Mesh.HIPS,
      trx.lara.ExtraMesh.DAGGER_HIPS
    )
  end
end)
