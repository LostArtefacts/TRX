-- The options page of the flat menu system: the settings dialogs, as the
-- pause screen and the title menu list them.

local list = require("common.ui.flat_menu.list")
local settings = require("common.ui.screens.settings")

local M = {}

trx.locale.declare({
  ["general/flat_menu/options"] = "Options",
})

-- The name of the page, as the row that leads to it shows it.
function M.title()
  return trx.locale.text("general/flat_menu/options")
end

-- Makes the page. Confirming a row runs `open` with the function that pushes
-- its dialog over a screen.
---@param open fun(push: fun(ctx: trx.ui.ScreenContext, on_close: fun()): trx.ui.StackLayer)
---@return common.ui.flat_menu.List
function M.page(open)
  return list.new(function()
    local rows = {}
    for _, entry in ipairs(settings.dialogs()) do
      rows[#rows + 1] = {
        text = entry.title,
        choose = function()
          open(entry.push)
        end,
      }
    end
    return { heading = M.title(), rows = rows }
  end)
end

return M
