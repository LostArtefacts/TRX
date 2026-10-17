-- The statistics of the flat menu system, as the original TR4 prints them
-- from its pause screen: the heading, the name of the level, and a line for
-- each counter with its value to the right of its name.

local stats = require("common.ui.screens.stats")
local TEXT_SCALE = require("common.ui.flat_menu.text").SCALE

local M = {}

local Role = trx.input.Role

-- The room between two lines, in canvas units.
local ROW_GAP = 2
-- Where the names and the values start, across the original's 4:3 screen of
-- 640 pixels, from its middle.
local KEY_X = -240
local VALUE_X = 80

local COLOR_HEADING = 6
local COLOR_KEY = 2
local COLOR_VALUE = 6

local function colored(color, text)
  return ("\\{color %d}%s"):format(color, text)
end

-- The x of a point the original gives across its 640-pixel screen.
local function across(x)
  local h = trx.ui.canvas.height
  return trx.ui.canvas.width / 2 + x * h * 4 / 3 / 640
end

local function line()
  local _, height = trx.ui.primitive.measure_text("Ag", TEXT_SCALE)
  return height + ROW_GAP
end

-- Shows the statistics of the level being played over a screen, in the
-- middle of it. Confirming or backing out closes them and runs `on_close`.
---@param ctx trx.ui.ScreenContext
---@param on_close fun()
---@return trx.ui.StackLayer?
function M.push(ctx, on_close)
  local title, rows = stats.level_rows()
  if rows == nil then
    return nil
  end
  local heading =
    colored(COLOR_HEADING, trx.locale.get("general/flat_menu/statistics"))

  local function paint()
    local primitive = trx.ui.primitive
    local w = trx.ui.canvas.width
    local step = line()
    -- The heading, a blank line and the name of the level stand above the
    -- rows.
    local top = trx.ui.canvas.height / 2 - step * (#rows + 3) / 2

    local function centred(text, y)
      local text_w = primitive.measure_text(text, TEXT_SCALE)
      primitive.text(text, (w - text_w) / 2, y, TEXT_SCALE)
    end

    -- The values start past the widest name, where the original's place for
    -- them is too close.
    local key_x = across(KEY_X)
    local value_x = across(VALUE_X)
    for _, row in ipairs(rows) do
      if row.key ~= nil then
        local key_w = primitive.measure_text(row.key .. "  ", TEXT_SCALE)
        value_x = math.max(value_x, key_x + key_w)
      end
    end

    centred(heading, top)
    centred(colored(COLOR_KEY, title or ""), top + step * 2)
    for i, row in ipairs(rows) do
      local y = top + step * (i + 2)
      if row.key ~= nil then
        primitive.text(colored(COLOR_KEY, row.key), key_x, y, TEXT_SCALE)
        primitive.text(
          colored(COLOR_VALUE, tostring(row.value)),
          value_x,
          y,
          TEXT_SCALE
        )
      else
        centred(colored(COLOR_KEY, row.text), y)
      end
    end
  end

  return ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return trx.ui.canvas.width, trx.ui.canvas.height
      end,
      paint = paint,
    }),
    place = function()
      return 0, 0
    end,
    on_input = function(layer, keys)
      if keys:pressed(Role.MENU_CONFIRM) or keys:pressed(Role.MENU_BACK) then
        layer:close()
        on_close()
      end
    end,
  })
end

return M
