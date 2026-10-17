-- The shipped window widget, which frames the game's dialogs.

local h = require("harness")
local test = h.test

local metrics = require("common.ui.metrics")
local scroll = require("common.ui.scroll")
local window = require("common.ui.widgets.window")

local function size(widget)
  trx.config.set("ui.text_scale", 1.0)
  local w, hh = widget:measure()
  return w / trx.ui.text_scale, hh / trx.ui.text_scale
end

local function near(a, b)
  return math.abs(a - b) < 0.01
end

local function content()
  return trx.ui.widgets.Spacer({ w = 50, h = 20 })
end

test("a window frames its content", function()
  local w, hh = size(window.new({ content = content() }))
  assert(near(w, 50 + window.chrome_width()), w)
  assert(near(hh, 20 + window.chrome_height({})), hh)
end)

test("a window without frames takes no padding", function()
  trx.ui.frames_shown = false
  local w, hh = size(window.new({ content = content() }))
  trx.ui.frames_shown = true
  assert(near(w, 50), w)
  assert(near(hh, 20), hh)
end)

test("a window over a list that fits keeps no room for arrows", function()
  local list = scroll.new({ count = 3, visible = 3 })
  local _, hh = size(window.new({ content = content(), scroll = list }))
  assert(near(hh, 20 + window.chrome_height({})), hh)
end)

test("a window over a long list keeps room for arrows", function()
  local list = scroll.new({ count = 5, visible = 3 })
  local root = window.new({ content = content(), scroll = list })
  local _, hh = size(root)
  assert(near(hh, 20 + window.chrome_height({ hints = true })), hh)
  list:set_count(3)
  _, hh = size(root)
  assert(near(hh, 20 + window.chrome_height({})), hh)
end)

test("a window can keep the room for arrows while its list fits", function()
  local list = scroll.new({ count = 1, visible = 3 })
  local _, hh = size(
    window.new({ content = content(), scroll = list, reserve_scroll = true })
  )
  assert(near(hh, 20 + 2 * metrics.HINT_HEIGHT + window.chrome_height({})), hh)
end)

test("a window's header and footer sit inside its frame", function()
  local root = window.new({
    content = content(),
    header = trx.ui.widgets.Spacer({ w = 0, h = 4 }),
    footer = trx.ui.widgets.Spacer({ w = 0, h = 6 }),
  })
  local _, hh = size(root)
  assert(near(hh, 30 + window.chrome_height({})), hh)
end)

return h.report()
