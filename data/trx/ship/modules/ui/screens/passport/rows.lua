-- The rows of the passport's lists: the save slots, the levels and the ways
-- to start a game.

local pages = require("common.ui.screens.passport.pages")

local M = {}

-- Lists every quick save and every numbered slot, empty or not. TR1 centers
-- the save number after the level name; the later games set it against the
-- right edge.
function M.slot_rows(filter)
  local rows = {}
  pages.each_slot(function(slot_num, pool)
    local info = trx.savegame.info(slot_num, pool)
    if filter ~= nil and not filter(info) then
      return
    end
    ---@type { slot_num: integer, pool: any, text: (string|trx.locale.Text)?, right: string? }
    local row = { slot_num = slot_num, pool = pool }
    if info == nil then
      row.text = trx.locale.text("general/misc/empty_slot_fmt", slot_num)
    elseif info.counter <= 0 then
      row.text = info.level_title
    elseif trx.game.tr_version == 1 then
      row.text = info.level_title .. " " .. info.counter
      if info.is_quick then
        row.text = row.text .. " (QS)"
      end
    else
      row.text = info.level_title
      row.right = info.is_quick and "QS " .. info.counter
        or tostring(info.counter)
    end
    rows[#rows + 1] = row
  end)
  return rows
end

-- Lists every level except the gym and the placeholder entries that only old
-- saves refer to.
function M.all_level_rows()
  local rows = {}
  for _, level in ipairs(trx.game.levels) do
    if
      level.type ~= trx.game.LevelType.GYM
      and level.type ~= trx.game.LevelType.DUMMY
      and level.type ~= trx.game.LevelType.CURRENT
    then
      rows[#rows + 1] = { text = trx.locale.level_title(level), level = level }
    end
  end
  return rows
end

function M.reached_level_rows(reached_nums)
  local reached = {}
  for _, num in ipairs(reached_nums) do
    reached[num] = true
  end
  local rows = {}
  for _, level in ipairs(trx.game.levels) do
    if level.type ~= trx.game.LevelType.GYM and reached[level.num] then
      rows[#rows + 1] = { text = trx.locale.level_title(level), level = level }
    end
  end
  return rows
end

function M.mode_rows()
  local rows = {
    {
      text = trx.locale.text("general/passport/mode_new_game"),
      choice = "new_game",
    },
  }
  if pages.game_modes_available() then
    rows[#rows + 1] = {
      text = trx.locale.text("general/passport/mode_new_game_plus"),
      choice = "new_game_plus",
    }
  end
  return rows
end

function M.new_game_rows()
  local rows = M.mode_rows()

  -- One setting turns both of the save choices on or off.
  local play_prev = false
  local story = false
  if
    not trx.config.get("flow.load_save_disabled")
    and trx.config.get("gameplay.enable_play_previous_levels")
  then
    pages.each_slot(function(slot_num, pool)
      local info = trx.savegame.info(slot_num, pool)
      if info == nil then
        return
      end
      play_prev = play_prev or info.can_select_level
      story = story or info.has_story
    end)
  end

  -- A rule separates these choices from the game modes.
  local extras = {}
  if play_prev then
    extras[#extras + 1] = {
      text = trx.locale.text("general/passport/play_previous_levels"),
      choice = "play_prev",
    }
  end
  if story then
    extras[#extras + 1] = {
      text = trx.locale.text("general/passport/story_so_far"),
      choice = "story",
    }
  end
  if #pages.switchable_mods() > 1 then
    extras[#extras + 1] = {
      text = trx.locale.text("general/passport/switch_mod"),
      choice = "switch_mod",
    }
  end
  for index, row in ipairs(extras) do
    row.rule = index == 1
    rows[#rows + 1] = row
  end
  return rows
end

return M
