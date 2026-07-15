local drink_frame_num = 737
local equipment_set = false

trx.events.after_control(function()
  local lara_item = trx.lara.item
  if lara_item.frame >= drink_frame_num and not equipment_set then
    trx.lara.set_extra_equipment(
      trx.lara.Mesh.HAND_R,
      trx.lara.ExtraMesh.DRINK_CAN
    )
    equipment_set = true
  end
end)
