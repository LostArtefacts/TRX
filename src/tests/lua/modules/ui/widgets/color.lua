-- The shipped color widgets: the swatch and the slider of the color editor.

local h = require("harness")
local test = h.test

local color = require("common.ui.widgets.color")

-- What a layer holding the widget draws.
local function scene_of(widget)
  trx.config.set("ui.text_scale", 1.0)
  local layer = trx.ui.layers.push({ root = widget })
  local scene = table.concat(fake.scene(), "\n")
  layer:close()
  return scene
end

test("a swatch draws its color inside a black box", function()
  local scene = scene_of(color.swatch({
    color = trx.math.color("123456"),
    w = 13,
    h = 13,
  }))
  assert(scene:match("color=000000"), scene)
  assert(scene:match("color=123456"), scene)
end)

test("a swatch follows the signal that carries its color", function()
  local value = trx.signal.new(trx.math.color("123456"))
  local swatch = color.swatch({ color = value, w = 13, h = 13 })
  value:set(trx.math.color("ABCDEF"))
  assert(scene_of(swatch):match("color=abcdef"))
end)

test("a slider draws a marker over its colors", function()
  local scene = scene_of(color.slider({
    stops = {
      trx.math.color("FF0000"),
      trx.math.color("00FF00"),
      trx.math.color("0000FF"),
    },
    value = 0.5,
    w = 100,
  }))
  assert(scene:match("color=ffffff"), scene)
  assert(scene:match("gradient"), scene)
end)

test("a swatch that is not shown draws nothing", function()
  local scene = scene_of(color.swatch({
    color = trx.math.color("123456"),
    w = 13,
    h = 13,
    visible = false,
  }))
  assert(not scene:match("color=123456"), scene)
end)

return h.report()
