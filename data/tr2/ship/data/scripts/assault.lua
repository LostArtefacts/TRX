trx.events.on_level_start(function(level)
  local records = trx.assault_stats.list_records()
  if #records > 1 then
    trx.music.play(22)
  else
    trx.music.play(5)
  end
end)
