-- Places overlay widgets by the settings that say where each goes.

local M = {}

trx.locale.declare({
  ["enums/ui_element_location/top_left"] = "Top left",
  ["enums/ui_element_location/top_center"] = "Top center",
  ["enums/ui_element_location/top_right"] = "Top right",
  ["enums/ui_element_location/bottom_left"] = "Bottom left",
  ["enums/ui_element_location/bottom_center"] = "Bottom center",
  ["enums/ui_element_location/bottom_right"] = "Bottom right",
})

-- The values of a setting that says where a widget goes, for its declaration.
M.LOCATIONS = {
  { value = "top_left", label = "enums/ui_element_location/top_left" },
  { value = "top_center", label = "enums/ui_element_location/top_center" },
  { value = "top_right", label = "enums/ui_element_location/top_right" },
  { value = "bottom_left", label = "enums/ui_element_location/bottom_left" },
  {
    value = "bottom_center",
    label = "enums/ui_element_location/bottom_center",
  },
  { value = "bottom_right", label = "enums/ui_element_location/bottom_right" },
}

-- Normalizes a value that the settings file saved with dashes, as it did
-- while the engine declared the setting, to the underscores that the values
-- take.
function M.normalize(key)
  local value = trx.config.get(key)
  if type(value) == "string" and value:find("-", 1, true) then
    trx.config.set(key, (value:gsub("-", "_")))
  end
end

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
