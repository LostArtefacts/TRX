trx.events.after_level_state(function()
  for _, room in pairs(trx.rooms) do
    room.damaging = room.underwater
    room.cold = true
  end
end)
