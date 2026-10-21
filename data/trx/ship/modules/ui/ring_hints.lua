-- The key hints and the count that the inventory ring shows over the caption
-- at the foot of the screen, and the name of the ring at the top. The ring
-- decides what they say, through the signals of `common.ui.ring`. The hints
-- and the count stack before the other widgets of their region, with a gap
-- between them and the caption.

local ring = require("common.ui.ring")

local M = {}

local SPACING = 20
local PART_SPACING = 60
local COUNT_SHIFT = 64
local CAPTION_GAP = 8

-- Builds the hints and the count and places them. Called once, as the
-- interface scripts load.
function M.setup()
  local ui = trx.ui
  local signals = ring.signals
  local hint = signals.hint
  local count = signals.count
  local heading = signals.heading

  local has_hint = hint:map(function(parts)
    return #parts > 0
  end)
  local has_count = count:map(function(text)
    return text ~= nil
  end)

  -- One label for each part the ring can show, of which the hint shows as many
  -- as it has.
  local parts = {}
  for i = 1, 4 do
    parts[i] = ui.widgets.Label({
      text = hint:map(function(texts)
        return texts[i] or ""
      end),
      visible = hint:map(function(texts)
        return texts[i] ~= nil
      end),
    })
  end
  -- Keeps the line of the hints while the ring shows a count and no hints,
  -- so that the count stays where it is as the hints come and go.
  parts[#parts + 1] = ui.widgets.Custom({
    visible = hint:map(function(texts)
      return #texts == 0
    end),
    measure = function()
      local text = ui.input_hint_text(trx.input.Role.MENU_CONFIRM, "")
      local _, h = ui.primitive.measure_text(text, 1.0)
      return 0, h
    end,
    paint = function() end,
  })

  -- The count stands to the right of the middle, where the ring draws it, and
  -- keeps the room of its text where it would stand centered. It reads the
  -- count itself rather than a signal made from it, so that it measures the
  -- new text on the tick that shows it.
  local shifted_count = ui.widgets.Custom({
    visible = has_count,
    measure = function()
      return ui.primitive.measure_text(count:get() or "", 1.0)
    end,
    paint = function(x, y)
      ui.primitive.text(
        count:get() or "",
        x + COUNT_SHIFT * ui.text_scale,
        y,
        1.0,
        0
      )
    end,
  })

  ui.regions.place(
    ui.Region.BOTTOM_CENTER,
    ui.widgets.Stack({
      spacing = SPACING,
      align = ui.HAlign.CENTER,
      children = {
        shifted_count,
        ui.widgets.Stack({
          orientation = ui.Orientation.HORIZONTAL,
          spacing = PART_SPACING,
          visible = has_hint | has_count,
          children = parts,
        }),
        ui.widgets.Spacer({
          h = CAPTION_GAP,
          visible = has_hint | has_count,
        }),
      },
    }),
    { order = -1 }
  )

  ui.regions.place(
    ui.Region.TOP_CENTER,
    ui.widgets.Label({
      text = heading:map(function(text)
        return text or ""
      end),
      visible = heading:map(function(text)
        return text ~= nil
      end),
    }),
    { order = 1 }
  )
end

return M
