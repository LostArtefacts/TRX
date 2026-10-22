-- The model that the controls item of the inventory shows, and the setting
-- that picks it. Each game ships three models for it: the Sega Saturn pad, a
-- keyboard and the PlayStation pad. Automatic shows a pad while the controls
-- are set to a controller, and the keyboard otherwise. The pad is the Saturn
-- one in TR1, the game that shipped with it, and the PlayStation one in the
-- other games.
--
-- Usage:
--   require("common.ui.controls_model").setup()
--   local object = require("common.ui.controls_model").object()

local M = {}

local SETTING = "visuals.controls_model"

local objects = trx.catalog.objects

-- Written out in full because the scanner that carries these into the
-- shipped strings reads a key only where it is a literal.
trx.locale.declare({
  ["settings/visuals.controls_model/title"] = "Controls model",
  ["settings/visuals.controls_model/description"] = "Which model the controls "
    .. "item of the inventory shows. Automatic shows a pad while the controls "
    .. "are set to a controller, and the keyboard otherwise. Original shows "
    .. "the model that each level ships.",
  ["settings/visuals.controls_model/values/auto"] = "Automatic",
  ["settings/visuals.controls_model/values/original"] = "Original",
  ["settings/visuals.controls_model/values/saturn"] = "Sega Saturn pad",
  ["settings/visuals.controls_model/values/keyboard"] = "Keyboard",
  ["settings/visuals.controls_model/values/ps1"] = "PlayStation pad",
})

local MODELS = {
  saturn = "controls_option_saturn",
  keyboard = "controls_option_keys",
  ps1 = "controls_option_ps1",
}

-- Returns the model that a choice of the setting shows.
---@param choice string
---@return string?
local function model_of(choice)
  if choice == "auto" then
    if trx.input.backend ~= trx.input.Backend.CONTROLLER then
      return MODELS.keyboard
    end
    return trx.game.version == 1 and MODELS.saturn or MODELS.ps1
  end
  return MODELS[choice]
end

-- Returns the object that the controls item draws. It is the item itself
-- where the setting asks for the original model, or where the level holds
-- no model of the choice.
---@return integer
function M.object()
  local key = model_of(trx.config.get(SETTING))
  local object = key ~= nil and objects[key] or nil
  if object == nil then
    return objects.CONTROLS_OPTION
  end
  local info = trx.objects[object]
  if info == nil or not info.loaded then
    return objects.CONTROLS_OPTION
  end
  return object
end

-- Declares the setting. Called once, from a global script.
function M.setup()
  trx.config.declare({
    key = SETTING,
    kind = "dynamic_enum",
    values = { "auto", "original", "saturn", "keyboard", "ps1" },
    default = "auto",
    ui = { tab = "graphic_ui", after = "ui.menu_system" },
  })
end

return M
