-- The shipped row arrows widget, which says a value can be stepped.

local h = require("harness")
local test = h.test

local row_arrows = require("common.ui.widgets.row_arrows")

local function width(widget)
  trx.config.set("ui.text_scale", 1.0)
  return (widget:measure())
end

test("the arrows keep their room when they hide", function()
  local left = trx.signal.new(true)
  local arrows = row_arrows.new({
    child = trx.ui.widgets.Label({ text = "On" }),
    left = left,
    right = true,
  })
  local shown = width(arrows)
  left:set(false)
  assert(width(arrows) == shown)
end)

test("wider spacing makes a wider row", function()
  local function make(spacing)
    return row_arrows.new({
      child = trx.ui.widgets.Label({ text = "On" }),
      spacing = spacing,
    })
  end
  assert(
    width(make(row_arrows.Spacing.WIDE))
      > width(make(row_arrows.Spacing.TIGHT))
  )
end)

return h.report()
