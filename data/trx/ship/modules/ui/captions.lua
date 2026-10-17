-- The lines of text and the arrows that the game asks for over the play: the
-- line at the top of the screen, the caption at the bottom with the arrows
-- beside it, the arrows in the corners, and the version on the title screen.
--
-- The game decides what they say, through `trx.overlay.signals`. They stack
-- after the other widgets of their regions.

local row_arrows = require("common.ui.widgets.row_arrows")
local timer = require("common.timer")

local M = {}

trx.locale.declare({
  ["settings/ui.show_title_version/title"] = "Title version text",
  ["settings/ui.show_title_version/description"] = "Shows the TRX version string in the title inventory ring.",
})

trx.config.declare({
  key = "ui.show_title_version",
  kind = "boolean",
  default = true,
})

local BLINK_PERIOD = 10
local VERSION_SCALE = 0.5
local ORDER = { order = 1 }

-- Shows the caption and hides it again while it flashes.
local function blinking(flashing)
  local hidden = trx.signal.new(false)
  ---@type common.timer.Timer?
  local running = nil
  local function follow(on)
    if on and running == nil then
      running = timer.every(BLINK_PERIOD, function()
        hidden:set(not hidden:get())
      end, { clock = timer.Clock.UI })
    elseif not on and running ~= nil then
      running:cancel()
      running = nil
      hidden:set(false)
    end
  end
  follow(flashing:get())
  flashing:on(follow)
  return hidden
end

local function text_of(text)
  return text:map(function(t)
    return t or ""
  end)
end

local function has(text)
  return text:map(function(t)
    return t ~= nil
  end)
end

-- Builds the texts and the arrows and places them. Called once, as the
-- interface scripts load.
function M.setup()
  local ui = trx.ui
  local overlay = trx.overlay
  local Arrow = overlay.Arrow
  local signals = overlay.signals

  local function shows(arrow)
    return signals.arrows:map(function(shown)
      return shown[arrow] == true
    end)
  end

  local top = signals.top_text
  ui.regions.place(
    ui.Region.TOP_CENTER,
    ui.widgets.Label({ text = text_of(top), visible = has(top) }),
    ORDER
  )

  local caption = signals.caption
  ui.regions.place(
    ui.Region.BOTTOM_CENTER,
    ui.widgets.Blank({
      visible = has(caption),
      when = blinking(signals.caption_flashing),
      child = row_arrows.new({
        left = shows(Arrow.CAPTION_LEFT),
        right = shows(Arrow.CAPTION_RIGHT),
        spacing = row_arrows.Spacing.WIDE,
        child = ui.widgets.Label({ text = text_of(caption) }),
      }),
    }),
    ORDER
  )

  ui.regions.place(
    ui.Region.BOTTOM_RIGHT,
    ui.widgets.Label({
      text = trx.game.TRX_VERSION,
      scale = VERSION_SCALE,
      visible = signals.version_shown
        & trx.signal.config("ui.show_title_version"),
    }),
    ORDER
  )

  -- An arrow in a corner takes the height of a bar, so that a corner keeps
  -- the room it has while it shows a bar. It shows only where nothing else
  -- stands in its corner.
  for arrow, place in pairs({
    [Arrow.TOP_LEFT] = { ui.Region.TOP_LEFT, "\\{arrow up}" },
    [Arrow.TOP_RIGHT] = { ui.Region.TOP_RIGHT, "\\{arrow up}" },
    [Arrow.BOTTOM_LEFT] = { ui.Region.BOTTOM_LEFT, "\\{arrow down}" },
    [Arrow.BOTTOM_RIGHT] = { ui.Region.BOTTOM_RIGHT, "\\{arrow down}" },
  }) do
    ui.regions.place(
      place[1],
      ui.widgets.Stack({
        orientation = ui.Orientation.HORIZONTAL,
        v_align = ui.VAlign.CENTER,
        visible = shows(arrow),
        children = {
          ui.widgets.Blank({
            when = true,
            child = ui.widgets.Bar({ w = 0 }),
          }),
          ui.widgets.Label({ text = place[2] }),
        },
      }),
      { order = 1, alone = true }
    )
  end
end

return M
