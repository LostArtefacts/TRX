-- Shows a selection of the player's pressed inputs per frame.

local KEY_GAP = 1
local KEY_PADDING = 2
local KEY_WIDTH = 28
local KEY_HEIGHT = 16

-- Background colors of the buttons.
local COLOR_UP = trx.math.color("#353535")
local COLOR_DOWN = trx.math.color("#c9c9c9")

local Role = trx.input.Role

-- Omitted `text` will show the game's button/name for the key. Use `colspan`
-- and/or `rowspan` for finer layout control.
local KEYS = {
  { x = 0, y = 0, role = Role.OPTION, text = "Inv" },
  { x = 1, y = 0, role = Role.USE_FLARE, text = "Flare" },

  { x = 3, y = 0, role = Role.STEP_LEFT, text = "SL" },
  { x = 4, y = 0, role = Role.ROLL, text = "Roll" },
  { x = 5, y = 0, role = Role.STEP_RIGHT, text = "SR" },

  { x = 0, y = 1, role = Role.SLOW, text = "Walk" },
  { x = 1, y = 1, role = Role.JUMP, text = "Jump" },
  { x = 2, y = 1, role = Role.SPRINT, text = "Dash" },
  { x = 3, y = 1, role = Role.CROUCH, text = "Duck" },
  { x = 4, y = 1, role = Role.FORWARD },
  { x = 5, y = 1, role = Role.LOOK, text = "Look" },

  { x = 0, y = 2, role = Role.ACTION, text = "Act" },
  { x = 1, y = 2, role = Role.DRAW, text = "Guns", colspan = 2 },
  { x = 3, y = 2, role = Role.LEFT },
  { x = 4, y = 2, role = Role.BACK },
  { x = 5, y = 2, role = Role.RIGHT },
}

local function get_colors()
  if trx.config.get("rendering.enable_wireframe") then
    return COLOR_UP, COLOR_DOWN
  end
  return COLOR_DOWN, COLOR_UP
end

local function get_key_span(key)
  return key.colspan or 1, key.rowspan or 1
end

local input_display = trx.ui.widgets.Custom({
  measure = function()
    local max_x = 0
    local max_y = 0
    for _, key in ipairs(KEYS) do
      local colspan, rowspan = get_key_span(key)
      max_x = math.max(max_x, key.x + colspan)
      max_y = math.max(max_y, key.y + rowspan)
    end

    local width = KEY_PADDING * 2 + max_x * KEY_WIDTH + (max_x - 1) * KEY_GAP
    local height = KEY_PADDING * 2 + max_y * KEY_HEIGHT + (max_y - 1) * KEY_GAP
    return width, height
  end,

  paint = function(x, y)
    local col_down, col_up = get_colors()

    for _, key in ipairs(KEYS) do
      local colspan, rowspan = get_key_span(key)

      local key_x = x + KEY_PADDING + key.x * (KEY_WIDTH + KEY_GAP)
      local key_y = y + KEY_PADDING + key.y * (KEY_HEIGHT + KEY_GAP)

      local width = colspan * KEY_WIDTH + (colspan - 1) * KEY_GAP
      local height = rowspan * KEY_HEIGHT + (rowspan - 1) * KEY_GAP

      local held = trx.input.is_held(key.role)
      trx.ui.primitive.quad(
        key_x,
        key_y,
        0,
        width,
        height,
        held and col_down or col_up
      )

      local label = key.text or trx.input.key_name(key.role)
      local scale = key.text ~= nil and 0.5 or 1
      local text_w, text_h = trx.ui.primitive.measure_text(label, scale)
      trx.ui.primitive.text(
        label,
        key_x + (width - text_w) / 2,
        key_y + (height - text_h) / 2,
        scale,
        1
      )
    end
  end,
})

trx.ui.regions.place(trx.ui.Region.BOTTOM_RIGHT, input_display)
