-- Drawing on top of the game.
--
-- Scripts build widgets and place them in regions. The primitive draw calls
-- beneath them belong to the engine and are only valid while a scene is being
-- painted. These tests cover widget measurement, stack layout, and paint-time
-- enforcement.

local h = require("harness")
local test, raises = h.test, h.raises

local widgets = trx.ui.widgets
local primitive = trx.ui.primitive

-- The fake text renderer is eight units wide per character and sixteen tall.
local function text_size(text, scale)
  return #text * 8 * (scale or 1), 16 * (scale or 1)
end

test("measuring text works outside a scene", function()
  local w, h_ = primitive.measure_text("iris")
  assert(w == select(1, text_size("iris")), w)
  assert(h_ == select(2, text_size("iris")), h_)
end)

test("a model reports the box it occupies", function()
  local obj = 5
  fake.set_mesh_bounds(obj, -10, -200, -30, 40, 5, 60)
  local min_x, min_y, min_z, max_x, max_y, max_z = primitive.mesh_bounds(obj)
  assert(min_x == -10, min_x)
  assert(min_y == -200, min_y)
  assert(min_z == -30, min_z)
  assert(max_x == 40, max_x)
  assert(max_y == 5, max_y)
  assert(max_z == 60, max_z)
end)

test("measuring a model the level did not load raises", function()
  raises(function()
    primitive.mesh_bounds(6)
  end, "not loaded")
end)

local function slot_line(lines)
  for _, op in ipairs(lines) do
    if op:match("^mesh_slot ") then
      return op
    end
  end
end

test("a model in a slot draws where it was moved", function()
  local slot = trx.ui.mesh_slot()
  slot:move({ object = 5, x = 10, y = 20, w = 30, h = 40, rot_y = 512 })
  assert(
    slot_line(fake.render(0))
      == "mesh_slot obj=5 x=10.0 y=20.0 w=30.0 h=40.0 rot_x=0 rot_y=512 rot_z=0 mask=4294967295"
  )
  slot:release()
end)

test("a model in a slot is blended between its two ticks", function()
  local slot = trx.ui.mesh_slot()
  slot:move({ object = 5, x = 0, y = 0, w = 10, h = 10, rot_y = 0 })
  slot:move({ object = 5, x = 100, y = 40, w = 10, h = 10, rot_y = 1000 })
  -- Half way between the two ticks, half way between the two poses.
  assert(
    slot_line(fake.render(0.5))
      == "mesh_slot obj=5 x=50.0 y=20.0 w=10.0 h=10.0 rot_x=0 rot_y=500 rot_z=0 mask=4294967295"
  )
  -- The fields report the tick, not the frame.
  assert(slot.x == 100, slot.x)
  slot:release()
end)

test("a model put down for the first time does not travel to it", function()
  local slot = trx.ui.mesh_slot()
  slot:move({ object = 5, x = 80, y = 0, w = 10, h = 10 })
  assert(
    slot_line(fake.render(0.5))
      == "mesh_slot obj=5 x=80.0 y=0.0 w=10.0 h=10.0 rot_x=0 rot_y=0 rot_z=0 mask=4294967295"
  )
  slot:release()
end)

test("a turn takes the short way around", function()
  local slot = trx.ui.mesh_slot()
  slot:move({ object = 5, x = 0, y = 0, w = 10, h = 10, rot_y = 0 })
  slot:move({ object = 5, x = 0, y = 0, w = 10, h = 10, rot_y = 65024 })
  -- 65024 is -512 the short way, so half way is -256, not 32512.
  local line = slot_line(fake.render(0.5))
  assert(line:match("rot_y=%-256 "), line)
  slot:release()
end)

test("a model in a slot holds the tilt and roll it was given", function()
  local slot = trx.ui.mesh_slot()
  slot:move({
    object = 5,
    x = 0,
    y = 0,
    w = 10,
    h = 10,
    rot_x = 16384,
    rot_y = 512,
    rot_z = -8192,
  })
  assert(
    slot_line(fake.render(0))
      == "mesh_slot obj=5 x=0.0 y=0.0 w=10.0 h=10.0 rot_x=16384 rot_y=512"
        .. " rot_z=-8192 mask=4294967295"
  )
  assert(slot.rot_x == 16384, slot.rot_x)
  assert(slot.rot_z == -8192, slot.rot_z)
  slot:release()
end)

test("a model in a slot draws only the meshes it was given", function()
  local slot = trx.ui.mesh_slot()
  slot:move({ object = 5, x = 0, y = 0, w = 10, h = 10, mesh_mask = 5 })
  local line = slot_line(fake.render(0))
  assert(line:match("mask=5$"), line)
  slot:release()
end)

test("a released slot's handle does not reach the next taker", function()
  local first = trx.ui.mesh_slot()
  first:release()
  local second = trx.ui.mesh_slot()
  second:move({ object = 5, x = 1, y = 2, w = 3, h = 4 })
  raises(function()
    first:move({ object = 5, x = 90, y = 90, w = 3, h = 4 })
  end, "stale")
  assert(
    slot_line(fake.render(0))
      == "mesh_slot obj=5 x=1.0 y=2.0 w=3.0 h=4.0 rot_x=0 rot_y=0 rot_z=0 mask=4294967295"
  )
  second:release()
end)

test("releasing a spent handle does nothing", function()
  local slot = trx.ui.mesh_slot()
  slot:release()
  slot:release()
end)

test("a hidden model draws nothing", function()
  local slot = trx.ui.mesh_slot()
  slot:move({ object = 5, x = 0, y = 0, w = 10, h = 10 })
  slot:hide()
  assert(slot_line(fake.render(0)) == nil)
  assert(not slot.visible)
  slot:release()
end)

test("nothing draws outside a painted scene", function()
  raises(function()
    primitive.text("iris", 0, 0)
  end, "painted")
  raises(function()
    primitive.quad(0, 0, 0, 1, 1, trx.math.color("#ffffff"))
  end, "painted")
end)

test("a label measures the text it holds", function()
  local label = widgets.Label({ text = "iris" })
  local w, h_ = label:measure()
  assert(w == select(1, text_size("iris")), w)
  assert(h_ == select(2, text_size("iris")), h_)
end)

test("a label follows the signal it was given", function()
  local text = trx.signal.new("iris")
  local label = widgets.Label({ text = text }):wakes_on(text)
  assert(label:measure() == select(1, text_size("iris")))
  text:set("scion")
  assert(label:measure() == select(1, text_size("scion")))
end)

test("a widget that is not shown keeps no room", function()
  local shown = trx.signal.new(false)
  local label = widgets.Label({ text = "iris", shown = shown })
  assert(not label:is_shown())
  shown:set(true)
  assert(label:is_shown())
end)

test("a stack adds up what it holds", function()
  local one = widgets.Label({ text = "a" })
  local two = widgets.Label({ text = "bb" })
  local stack = widgets.Stack({ children = { one, two }, spacing = 4 })
  local w, h_ = stack:measure()
  assert(w == 2 * 8, w)
  assert(h_ == 16 + 4 + 16, h_)
end)

test("a stack leaves no gap where a child is not shown", function()
  local gone = trx.signal.new(false)
  local stack = widgets.Stack({
    spacing = 4,
    children = {
      widgets.Label({ text = "a" }),
      widgets.Label({ text = "bb", shown = gone }),
    },
  })
  assert(select(2, stack:measure()) == 16, "the hidden child kept its room")
end)

test("a stack runs the other way when it is told to", function()
  local stack = widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    spacing = 2,
    children = {
      widgets.Label({ text = "a" }),
      widgets.Label({ text = "bb" }),
    },
  })
  local w, h_ = stack:measure()
  assert(w == 8 + 2 + 16, w)
  assert(h_ == 16, h_)
end)

test("a child growing grows the stack around it", function()
  local text = trx.signal.new("a")
  local child = widgets.Label({ text = text }):wakes_on(text)
  local stack = widgets.Stack({ children = { child } })
  assert(stack:measure() == 8)
  text:set("abcd")
  assert(stack:measure() == 32, "the stack kept the size it first took")
end)

test("a resized widget takes the size it was given", function()
  local box = widgets.Resize({
    w = 100,
    h = 50,
    child = widgets.Label({ text = "iris" }),
  })
  local w, h_ = box:measure()
  assert(w == 100, w)
  assert(h_ == 50, h_)
end)

test("a row keeps the room its arrows take", function()
  local body = widgets.Label({ text = "iris" })
  local lit = widgets.Row({ child = body, left = true, right = true })
  local dark = widgets.Row({ child = body, left = false, right = false })
  assert(lit:measure() == dark:measure(), "the arrows moved what they flank")
end)

-- Bar borders and padding must keep the same thickness on every side at every
-- bar scale. Drawn rectangles round both edges, so these checks verify the
-- final scheduled pixels rather than the canvas-space dimensions.
local function quads_of(ops)
  local out = {}
  for _, line in ipairs(ops) do
    local x, y, w, hh =
      line:match("^quad x=(%-?%d+) y=(%-?%d+) z=%-?%d+ w=(%-?%d+) h=(%-?%d+)")
    if x ~= nil then
      out[#out + 1] = {
        x = tonumber(x),
        y = tonumber(y),
        w = tonumber(w),
        h = tonumber(hh),
      }
    end
  end
  return out
end

test("a direct quad can be recorded", function()
  local ops = fake.paint(function()
    trx.ui.primitive.quad(0, 0, 0, 10, 10, trx.math.color("#ff0000"))
  end)
  assert(#ops > 0, "direct quad recorded " .. #ops)
end)

for _, scale in ipairs({ 0.5, 1.0, 1.3, 1.75, 2.0 }) do
  test(("a bar borders evenly at bar scale %s"):format(scale), function()
    -- Use a viewport that does not divide evenly into the canvas, so edges
    -- land on fractional pixels before rounding.
    fake.set_viewport(1707, 963)
    trx.config.set("ui.bar_scale", scale)
    local bar = widgets.Bar({ type = trx.ui.BarType.LARA_HP, value = 0.5 })
    local w, hh = bar:measure()

    -- Try several placements because the bar's position affects how each edge
    -- rounds to screen pixels.
    for step = 0, 23 do
      local at_x, at_y = 12 + step * 0.25, 7 + step * 0.3
      local quads = quads_of(fake.paint(function()
        bar:paint(at_x, at_y, w, hh)
      end))
      assert(#quads >= 3, ("only %d quads"):format(#quads))

      -- The first quad is the outer border and the third is the black inside;
      -- their difference is the visible border thickness.
      local outer, inner = quads[1], quads[3]
      local left = inner.x - outer.x
      local right = (outer.x + outer.w) - (inner.x + inner.w)
      local top = inner.y - outer.y
      local bottom = (outer.y + outer.h) - (inner.y + inner.h)
      assert(
        left == right,
        ("at %s,%s left %d and right %d differ"):format(
          at_x,
          at_y,
          left,
          right
        )
      )
      assert(
        top == bottom,
        ("at %s,%s top %d and bottom %d differ"):format(
          at_x,
          at_y,
          top,
          bottom
        )
      )
    end
  end)
end

test(
  "a level script's widget leaves its region when the level ends",
  function()
    local widget
    fake.as_level_script(function()
      widget = widgets.Label({ text = "iris" })
      trx.ui.regions.place(trx.ui.Region.TOP_CENTER, widget)
    end)

    fake.end_level()
    assert(
      not trx.ui.regions.remove(widget),
      "it was already out of the region"
    )
  end
)

test("a global script's widget stays in its region across a level", function()
  local widget = widgets.Label({ text = "iris" })
  trx.ui.regions.place(trx.ui.Region.TOP_CENTER, widget)

  fake.end_level()
  assert(trx.ui.regions.remove(widget), "it was still in the region")
end)

-- The engine takes trxc off the globals once the API is sealed, so a module
-- that reads it when a script calls in, rather than when the module loads,
-- raises at the worst moment.
test("a widget is placed after trxc leaves the globals", function()
  local capi = trxc
  trxc = nil
  local widget = widgets.Label({ text = "iris" })
  local ok, err = pcall(trx.ui.regions.place, trx.ui.Region.TOP_LEFT, widget)
  trxc = capi
  assert(ok, err)
  trx.ui.regions.remove(widget)
end)

fake.set_viewport(640, 480)

test("the clipboard holds what a script puts there", function()
  trx.ui.clipboard = "3571"
  assert(trx.ui.clipboard == "3571")

  trx.ui.clipboard = ""
  assert(trx.ui.clipboard == "", "an empty clipboard reads as an empty string")
end)

test("the layer over the interface paints after the layer under it", function()
  local under = trx.events.on_ui_paint(function()
    trx.ui.primitive.quad(0, 0, 0, 10, 10, trx.math.color("#ff0000"))
  end)
  local over = trx.events.on_ui_paint_over(function()
    trx.ui.primitive.quad(0, 0, 0, 10, 10, trx.math.color("#00ff00"))
  end)

  local ops = fake.scene()
  under:detach()
  over:detach()

  local under_at, over_at
  for i, op in ipairs(ops) do
    if op:find("ff0000", 1, true) then
      under_at = i
    elseif op:find("00ff00", 1, true) then
      over_at = i
    end
  end

  assert(under_at ~= nil, "the layer under the interface did not paint")
  assert(over_at ~= nil, "the layer over the interface did not paint")
  assert(over_at > under_at, "the layers painted in the wrong order")
end)

test("a layer keeps its own widgets", function()
  local over = trx.ui.widgets.Label({ text = "only-over" })
  trx.ui.regions.place(trx.ui.Region.BOTTOM_RIGHT, over, trx.ui.Layer.OVER)
  assert(trx.ui.regions.remove(over) == true)
  assert(trx.ui.regions.remove(over) == false, "it was removed twice")
end)

return h.report()
