-- Places overlay widgets by the settings that say where each goes.

local M = {}

local REGION_OF = {
  top_left = trx.ui.Region.TOP_LEFT,
  top_center = trx.ui.Region.TOP_CENTER,
  top_right = trx.ui.Region.TOP_RIGHT,
  bottom_left = trx.ui.Region.BOTTOM_LEFT,
  bottom_center = trx.ui.Region.BOTTOM_CENTER,
  bottom_right = trx.ui.Region.BOTTOM_RIGHT,
}

-- Places a widget in the region that a setting names. The widget follows the
-- setting, so moving it in the options moves it on screen.
function M.at_setting(key, widget)
  trx.ui.regions.place(
    trx.signal.config(key):map(function(location)
      return REGION_OF[location] or trx.ui.Region.TOP_LEFT
    end),
    widget
  )
end

return M
