-- Willard fires twice at the start of the scene, but the scene carries no
-- effect command for it, so the flashes are placed on the cutscene frames his
-- arm recoils on and the shots are heard on.
local willard_shots = {
  [3147] = true,
  [3155] = true,
}

-- Willard's gun is part of his hand mesh, with the barrel along the joint's
-- Y axis. The offset moves the flash from the joint to the muzzle.
local willard_gun_joint = 13
local willard_muzzle = { x = 0, y = 156, z = 45 }

trx.events.after_control(function()
  local frame_num = trx.game.cutscene_frame
  if frame_num == nil or not willard_shots[frame_num] then
    return
  end

  local willard =
    trx.items.query:of_object(trx.catalog.objects.player_2):first()
  if willard ~= nil then
    trx.fx.gun_flash(
      willard,
      { mesh = willard_gun_joint, pos = willard_muzzle }
    )
  end
end)
