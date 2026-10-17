-- The sounds the original TR4 plays in its menus, by the names the samples
-- of each game take.

local M = {}

-- Plays a sound: `rotate` as a row turns, `select` to move along the options
-- of an entry or back out of a list, `move` to move along a list, `choose`
-- to choose, `combine` as two entries combine, and `no` where nothing can be
-- done. TR1 takes the short sample of a turning ring for `select` and
-- `move`, as its own menu sample is the long one the ring spins in with.
function M.play(name)
  local samples = trx.catalog.samples
  local tr1 = trx.game.version == 1
  local tr4 = trx.game.version == 4
  local choose = tr4 and samples.MENU_SPININ or samples.MENU_CHOOSE
  local sounds = {
    rotate = samples.MENU_ROTATE,
    select = tr1 and samples.MENU_ROTATE or samples.MENU_CHOOSE,
    move = tr1 and samples.MENU_ROTATE or choose,
    choose = choose,
    combine = tr4 and samples.MENU_GUNS or samples.MENU_CHOOSE,
    no = samples.LARA_NO,
  }
  local sample = sounds[name]
  if sample ~= nil then
    trx.sound.play(sample)
  end
end

return M
