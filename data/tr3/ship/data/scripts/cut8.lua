local equipment_set = false

trx.events.after_control(function()
  local lara_item = trx.lara.item
  if lara_item.frame >= 737 and not equipment_set then
    trx.lara.set_extra_equipment(10, 10)
    equipment_set = true
  end
end)
