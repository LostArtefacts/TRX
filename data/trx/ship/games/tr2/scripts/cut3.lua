local suit_change_anim = 7
local outfit_changed = false

-- Bartoli shoots the monk from the ledge above, and Lara answers with her
-- pistols. The scene carries no effect command for any of it, so the flashes
-- are placed by the cutscene frames his firing animation recoils on, and by
-- the frames her shots are heard on.
local bartoli_shots = {
  [3374] = true,
  [3383] = true,
  [3536] = true,
  [3544] = true,
}
local lara_shots = {
  [3387] = "right",
  [3396] = "right",
  [3402] = "left",
  [3412] = "right",
  [3414] = "left",
  [3548] = "right",
  [3575] = "right",
  [3607] = "right",
  [3613] = "left",
}

-- Bartoli holds his gun at the end of the arm he points with, which is the
-- last joint of that chain rather than a hand as Lara's model has. The
-- offset moves the flash from the joint to the muzzle, clear of the arm.
local bartoli_gun_joint = 17
local bartoli_muzzle = { x = 0, y = 120, z = 0 }
local lara_hands = { right = 10, left = 13 }

trx.events.after_control(function()
  local lara_item = trx.lara.item
  if lara_item.anim_num >= suit_change_anim and not outfit_changed then
    trx.lara.outfit = "tr2_diving_suit"
    outfit_changed = true
  end

  local frame_num = trx.game.cutscene_frame
  if frame_num == nil then
    return
  end

  if bartoli_shots[frame_num] then
    local bartoli =
      trx.items.query:of_object(trx.catalog.objects.player_6):first()
    if bartoli ~= nil then
      trx.fx.gun_flash(
        bartoli,
        { mesh = bartoli_gun_joint, pos = bartoli_muzzle }
      )
    end
  end

  local hand = lara_shots[frame_num]
  if hand ~= nil then
    local flash = trx.weapons["pistols"].flash
    trx.fx.gun_flash(lara_item, {
      mesh = lara_hands[hand],
      pos = flash.pos.right,
    })
  end
end)
