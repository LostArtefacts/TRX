-- The key hints and the count that the inventory ring shows over the caption
-- at the foot of the screen. The ring decides what they say, through
-- `trx.inventory_ring.signals`. They stack before the other widgets of their
-- region, with a gap between them and the caption.

local M = {}

local SPACING = 20
local PART_SPACING = 60
local COUNT_SHIFT = 64
local CAPTION_GAP = 8

-- Builds the hints and the count and places them. Called once, as the
-- interface scripts load.
function M.setup()
  local ui = trx.ui
  local signals = trx.inventory_ring.signals
  local hint = signals.hint
  local count = signals.count

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

  -- The count stands to the right of the middle, where the ring draws it, and
  -- keeps the room of its text where it would stand centered.
  local count_text = count:map(function(text)
    return text or ""
  end)
  local shifted_count = ui.widgets.Custom({
    visible = has_count,
    measure = function()
      return ui.primitive.measure_text(count_text:get(), 1.0)
    end,
    paint = function(x, y)
      ui.primitive.text(
        count_text:get(),
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
        ui.widgets.Stack({
          orientation = ui.Orientation.HORIZONTAL,
          spacing = PART_SPACING,
          visible = has_hint,
          children = parts,
        }),
        shifted_count,
        ui.widgets.Spacer({
          h = CAPTION_GAP,
          visible = has_hint | has_count,
        }),
      },
    }),
    { order = -1 }
  )
end

return M
