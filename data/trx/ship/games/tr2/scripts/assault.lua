require("common.assault")

trx.events.on_game_start(function(is_save)
  trx.lara.holsters_visible = trx.lara.has_pistol_weapon
  if is_save then
    return
  end
  local records = trx.assault.stats.list_records()
  if #records > 1 then
    trx.music.tracks[22]:play()
  else
    trx.music.tracks[5]:play()
  end
end)
