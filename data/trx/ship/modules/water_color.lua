-- The underwater tint each release shipped with, offered as a setting.
--
-- A game calls declare() with its own colors; the colors themselves are the
-- ones docs/trx/WATER_COLORS.md lists. A mode is worth either one color for the
-- whole game or, where a release tinted the water per level as the PS1 ones
-- did, a table of colors keyed by trx.game.Level.key.

-- The setting this declares, and the engine's own color a preset holds while
-- it is picked.
local MODE_OPTION = "visuals.water_color_mode"
local COLOR_OPTION = "visuals.water_color"

-- Every value any game offers, declared here once: the text is the same
-- wherever it is shown, and a game lists in `order` only the ones it has.
-- Written out in full because the scanner that carries these into the shipped
-- strings reads a key only where it is a literal.
trx.locale.declare({
  ["settings/visuals.water_color_mode/title"] = "Water color preset",
  ["settings/visuals.water_color_mode/description"] = "Which release's underwater tint to use. "
    .. "While a preset is picked it holds the water color below; Custom leaves that color to you.",
  ["settings/visuals.water_color_mode/values/custom"] = "Custom",
  ["settings/visuals.water_color_mode/values/dos"] = "DOS",
  ["settings/visuals.water_color_mode/values/pc"] = "PC",
  ["settings/visuals.water_color_mode/values/pc_hardware"] = "PC (hardware)",
  ["settings/visuals.water_color_mode/values/pc_software"] = "PC (software)",
  ["settings/visuals.water_color_mode/values/ps1"] = "PS1",
  ["settings/visuals.water_color_mode/values/tombati"] = "TombATI",
})

-- A preset holds the color rather than writing it: the player's own color stays
-- underneath, comes back with Custom, and the color row is greyed while the
-- hold stands. Only ever one override is on, so the last is lifted before the
-- next goes down.
local held = false

local function hold(color)
  -- The flag moves before the call rather than after it: a watcher hears the
  -- setting move from inside these, so this runs again before either returns,
  -- and a nested call that read a stale flag would leave its hold behind.
  if held then
    held = false
    trx.config.restore(COLOR_OPTION)
  end
  if color ~= nil then
    held = true
    trx.config.override(COLOR_OPTION, color)
  end
end

local water_color = {}

function water_color.declare(spec)
  local per_level = false
  for _, mode in ipairs(spec.order) do
    if type(spec.modes[mode]) == "table" then
      per_level = true
    end
  end

  trx.config.declare({
    key = MODE_OPTION,
    kind = "dynamic_enum",
    values = spec.order,
    default = "custom",
    ui = {
      tab = "graphic_visuals",
      before = COLOR_OPTION,
    },
  })

  local function apply(mode)
    local color = spec.modes[mode]
    if type(color) == "table" then
      local level = trx.game.current_level
      color = level ~= nil and color[level.key] or nil
    end
    hold(color)
  end

  -- The watcher hears the saved mode as it attaches, so a preset the player
  -- picked last session is in force before the first level loads.
  trx.config.on_change(MODE_OPTION, apply)

  if per_level then
    -- The color follows the level, so it is picked again as each one starts.
    trx.events.on_game_start(function()
      apply(trx.config.get(MODE_OPTION))
    end)
  end
end

return water_color
