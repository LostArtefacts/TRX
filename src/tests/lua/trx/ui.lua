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

local function model_line(lines)
  for _, op in ipairs(lines) do
    if op:match("^model ") then
      return op
    end
  end
end

local function model_lines(lines)
  local result = {}
  for _, op in ipairs(lines) do
    if op:match("^model ") then
      result[#result + 1] = op
    end
  end
  return result
end

test("a model draws in its box at its depth", function()
  local line = model_line(fake.paint(function()
    primitive.model(5, 10, 20, 30, 40, { rot = { x = 64, y = 512, z = 128 } })
  end))
  assert(
    line
      == "model obj=5 x=10 y=20 w=30 h=40 rot_x=64 rot_y=512 rot_z=128 mask=4294967295 lit=0 held=1",
    line
  )
end)

test("a model draws only the meshes it was given", function()
  local line = model_line(fake.paint(function()
    primitive.model(5, 0, 0, 10, 10, { mesh_mask = 5 })
  end))
  assert(line:match(" mask=5 "), line)
end)

test("a model in a world of its own draws with its camera and pose", function()
  local lines = fake.paint(function()
    primitive.world_model(5, {
      camera = {
        pos = { x = 0, y = -256, z = 1286 },
        target = { x = 0, y = -96, z = 688 },
        fov = 80 * trx.math.DEG_1,
      },
      pose = {
        { turn = { x = 0, y = 16384, z = 0 } },
        { move = { x = 688, y = 0, z = 0 } },
      },
      mesh_mask = 3,
      light = {
        ambient = { r = 32, g = 32, b = 32 },
        lights = {
          {
            dir = { x = 0, y = 0, z = 1 },
            color = { r = 255, g = 255, b = 255 },
          },
        },
        meshes = { [0] = { r = 0, g = 15.9375, b = 239.0625 } },
      },
    })
  end)
  local found = nil
  for _, op in ipairs(lines) do
    if op:match("^world_model ") then
      found = op
    end
  end
  assert(
    found == "world_model obj=5 mask=3 lit=1 lights=1 steps=2 fov=14560",
    tostring(found)
  )
end)

test("a pose takes up to sixteen steps", function()
  local pose = {}
  for i = 1, 17 do
    pose[i] = { move = { x = i, y = 0, z = 0 } }
  end
  raises(function()
    fake.paint(function()
      primitive.world_model(5, {
        camera = {
          pos = { x = 0, y = 0, z = 0 },
          target = { x = 0, y = 0, z = 1 },
          fov = 0,
        },
        pose = pose,
      })
    end)
  end)
end)

test("darkening and a background go behind the interface", function()
  local lines = fake.paint(function()
    primitive.background(nil, true, 0.5)
    primitive.darken(0.25)
  end)
  local text = table.concat(lines, "\n")
  assert(text:match("background path=%- mono=1 dim=0.50"), text)
  assert(text:match("darken opacity=0.250"), text)
end)

test("a model widget fills its box at the player's text size", function()
  local model = widgets.Model({ object = 5, w = 30, h = 20 })
  local w, h_ = model:measure()
  assert(w == 30 and h_ == 20, w .. "x" .. h_)
  local line = model_line(fake.paint(function()
    model:paint(100, 50, w, h_)
  end))
  assert(line:match(" x=100 y=50 w=30 h=20 "), line)
end)

test("a model widget draws smaller within its box", function()
  local model = widgets.Model({ object = 5, w = 40, h = 20, size = 0.5 })
  local line = model_line(fake.paint(function()
    model:paint(0, 0, 40, 20)
  end))
  assert(line:match(" x=10 y=5 w=20 h=10 "), line)
end)

test("a model widget sized to nothing draws nothing", function()
  local model = widgets.Model({ object = 5, w = 40, h = 20, size = 0 })
  assert(#model_lines(fake.paint(function()
    model:paint(0, 0, 40, 20)
  end)) == 0)
end)

test("a spinning model widget turns between ticks", function()
  local model = widgets.Model({ object = 5, w = 10, h = 10, spin = 100 })
  local function turn()
    local line = model_line(fake.paint(function()
      model:paint(0, 0, 10, 10)
    end))
    return tonumber(line:match(" rot_y=(%-?%d+) "))
  end
  fake.set_rate(0)
  local before = turn()
  fake.set_rate(0.5)
  local half = turn()
  fake.set_rate(1)
  local after = turn()
  assert(half - before == 50, before .. " " .. half)
  assert(after - before == 100, before .. " " .. after)
end)

test("nothing draws outside a painted scene", function()
  raises(function()
    primitive.text("iris", 0, 0)
  end, "painted")
  raises(function()
    primitive.quad(0, 0, 0, 1, 1, trx.math.color("#ffffff"))
  end, "painted")
  raises(function()
    primitive.model(5, 0, 0, 10, 10)
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
  local label = widgets.Label({ text = text })
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
  local child = widgets.Label({ text = text })
  local stack = widgets.Stack({ children = { child } })
  assert(stack:measure() == 8)
  text:set("abcd")
  assert(stack:measure() == 32, "the stack kept the size it first took")
end)

test("a child shown again takes its room back in the stack", function()
  local shown = trx.signal.new(false)
  local stack = widgets.Stack({
    spacing = 4,
    children = {
      widgets.Label({ text = "a" }),
      widgets.Label({ text = "bb", shown = shown }),
    },
  })
  assert(select(2, stack:measure()) == 16)
  shown:set(true)
  assert(select(2, stack:measure()) == 16 + 4 + 16, "the child kept no room")
end)

test("a signal registered twice detaches with the widget", function()
  local text = trx.signal.new("a")
  local label = widgets.Label({ text = text }):wakes_on(text, text)
  label:measure()
  assert(label:release(), "the label listened to nothing")
  text:set("abcd")
  assert(not label:release(), "a listener outlived the release")
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
  local under = trx.ui.on_paint(function()
    trx.ui.primitive.quad(0, 0, 0, 10, 10, trx.math.color("#ff0000"))
  end)
  local over = trx.ui.on_paint_over(function()
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

test("the layer under the interface draws behind it", function()
  local under = trx.ui.on_paint(function()
    trx.ui.primitive.quad(0, 0, 0, 10, 10, trx.math.color("#ff0000"))
  end)
  local over = trx.ui.on_paint_over(function()
    trx.ui.primitive.quad(0, 0, 0, 10, 10, trx.math.color("#00ff00"))
  end)

  local ops, layers = fake.scene(), fake.scene_layers()
  under:detach()
  over:detach()

  local under_layer, over_layer
  for i, op in ipairs(ops) do
    if op:find("ff0000", 1, true) then
      under_layer = layers[i]
    elseif op:find("00ff00", 1, true) then
      over_layer = layers[i]
    end
  end
  assert(under_layer == "under", tostring(under_layer))
  assert(over_layer == "over", tostring(over_layer))
end)

local function quad_z(ops)
  for _, op in ipairs(ops) do
    local z = op:match(" z=(%-?%d+)")
    if z ~= nil then
      return tonumber(z)
    end
  end
end

test("a depth pushed past the near plane still draws", function()
  local ops = fake.paint(function()
    trx.ui.primitive.quad(0, 0, -50, 10, 10, trx.math.color("#ff0000"))
  end)
  assert(quad_z(ops) == 0, quad_z(ops))
end)

test("a layer keeps its own widgets", function()
  local over = trx.ui.widgets.Label({ text = "only-over" })
  trx.ui.regions.place(trx.ui.Region.BOTTOM_RIGHT, over, trx.ui.Layer.OVER)
  assert(trx.ui.regions.remove(over) == true)
  assert(trx.ui.regions.remove(over) == false, "it was removed twice")
end)

test("a custom widget measures and draws through its functions", function()
  local painted
  local custom = widgets.Custom({
    measure = function()
      return 30, 20
    end,
    paint = function(x, y, w, h_)
      painted = { x, y, w, h_ }
    end,
  })
  local w, h_ = custom:measure()
  assert(w == 30 and h_ == 20)
  custom:paint(5, 6, 30, 20)
  assert(painted[1] == 5 and painted[2] == 6)
  assert(painted[3] == 30 and painted[4] == 20)
end)

test("a custom widget that is not shown draws nothing", function()
  local painted = false
  local custom = widgets.Custom({
    measure = function()
      return 1, 1
    end,
    paint = function()
      painted = true
    end,
    shown = false,
  })
  custom:paint(0, 0, 1, 1)
  assert(not painted)
end)

local function rows(...)
  local result = {}
  for i, text in ipairs({ ... }) do
    result[i] = { text = text }
  end
  return result
end

local function with_wraparound(enabled, fn)
  trx.config.set("ui.enable_wraparound", enabled)
  local ok, err = pcall(fn)
  trx.config.set("ui.enable_wraparound", true)
  assert(ok, err)
end

test("a list measures its widest row and every row", function()
  local w, h_ = widgets.List({ rows = rows("a", "bb", "ccc") }):measure()
  assert(w == 3 * 8 + 2 * 4, w)
  assert(h_ == 3 * 15 + 2 * 3, h_)
end)

test("a list keeps its least width", function()
  local w = widgets.List({ rows = rows("a"), width = 100 }):measure()
  assert(w == 100, w)
end)

test("a list that runs past its rows keeps room for the arrows", function()
  local list =
    widgets.List({ rows = rows("a", "b", "c", "d", "e"), visible = 2 })
  local _, h_ = list:measure()
  assert(h_ == 2 * 15 + 3 + 2 * 7, h_)
end)

test("a list that keeps room keeps it for rows it does not hold", function()
  local list = widgets.List({ rows = rows("a"), visible = 3, reserve = true })
  local _, h_ = list:measure()
  assert(h_ == 3 * 15 + 2 * 3, h_)
end)

test("a rule above a row takes room", function()
  local list = widgets.List({
    rows = { { text = "a" }, { text = "b", rule = true } },
  })
  local _, h_ = list:measure()
  assert(h_ == 2 * 15 + 3 + 10, h_)
end)

test("a list keeps one height while a rule scrolls out of view", function()
  local list = widgets.List({
    rows = { { text = "a" }, { text = "b", rule = true }, { text = "c" } },
    visible = 1,
  })
  local _, before = list:measure()
  list:select(3)
  local _, after = list:measure()
  assert(before == after, ("%s ~= %s"):format(before, after))
end)

test("a list follows the signal that holds its visible rows", function()
  local visible = trx.signal.new(3)
  local list = widgets.List({ rows = rows("a", "b", "c"), visible = visible })
  local _, before = list:measure()
  visible:set(1)
  local _, after = list:measure()
  assert(before == 3 * 15 + 2 * 3, before)
  assert(after == 15 + 2 * 7, after)
  list:release()
end)

test("a list stops at its ends without wraparound", function()
  with_wraparound(false, function()
    local list = widgets.List({ rows = rows("a", "b") })
    assert(list:move(-1) == false)
    assert(list:selection() == 1)
    list:select(2)
    assert(list:move(1) == false)
    assert(list:selection() == 2)
  end)
end)

test("a list wraps around where the setting says so", function()
  with_wraparound(true, function()
    local list = widgets.List({ rows = rows("a", "b", "c") })
    assert(list:move(-1) == true)
    assert(list:selection() == 3)
  end)
end)

test(
  "a list does not report a move that leaves the cursor in place",
  function()
    with_wraparound(true, function()
      assert(widgets.List({ rows = rows("a") }):move(1) == false)
      assert(widgets.List({ rows = rows("a", "b") }):move(0) == false)
    end)
  end
)

test("a list keeps the cursor within new rows", function()
  local list = widgets.List({ rows = rows("a", "b", "c") })
  list:select(3)
  list:set_rows(rows("a"))
  assert(list:selection() == 1)
end)

test("an empty list has no selection", function()
  local list = widgets.List({})
  assert(list:selection() == nil)
  list:set_rows(rows("a"))
  assert(list:selection() == 1)
end)

local function ticks(count)
  for _ = 1, count do
    fake.tick()
  end
end

test("the curves start at nothing and end at the whole way", function()
  for _, name in ipairs({ "linear", "quad_in", "quad_out", "quad_in_out" }) do
    local curve = trx.ui.ease[name]
    assert(curve(0) == 0, name)
    assert(curve(1) == 1, name)
  end
  assert(trx.ui.ease.quad_in(0.5) < 0.5)
  assert(trx.ui.ease.quad_out(0.5) > 0.5)
  assert(trx.ui.ease.quad_in_out(0.5) == 0.5)
end)

test("a tween rests where it was made", function()
  local tween = trx.ui.tween(3)
  assert(tween:get() == 3 and tween:target() == 3)
  assert(not tween:is_moving())
end)

-- How many listeners a signal has.
local function listeners_of(signal)
  local count = 0
  for _ in pairs(rawget(signal, "_listeners")) do
    count = count + 1
  end
  return count
end

test("a fitted tree shrinks below its first level", function()
  local rows = {}
  for i = 1, 60 do
    rows[i] = trx.ui.widgets.Label({ text = "row " .. i })
  end
  local fitted = trx.ui.widgets.Fit({
    child = trx.ui.widgets.Pad({
      x = 4,
      y = 4,
      child = trx.ui.widgets.Stack({ children = rows }),
    }),
  })
  local _, h = fitted:measure()
  assert(h <= trx.ui.canvas.height, "the tree kept its full height: " .. h)
  fitted:release()
end)

test("releasing a widget releases every widget under it", function()
  local text = trx.signal.new("deep")
  local tree = trx.ui.widgets.Pad({
    x = 1,
    y = 1,
    child = trx.ui.widgets.Stack({
      children = { trx.ui.widgets.Label({ text = text }) },
    }),
  })
  tree:measure()
  assert(listeners_of(text) == 1, listeners_of(text))
  assert(tree:release(), "the tree said it held nothing")
  assert(listeners_of(text) == 0, "a widget under the tree kept listening")
  assert(not tree:release(), "a second release found more to release")
end)

test("a tween arrives after the ticks it was given", function()
  fake.set_rate(1)
  local tween = trx.ui.tween(0):to(8, 4)
  assert(tween:is_moving())
  ticks(2)
  assert(tween:get() == 4, tween:get())
  ticks(2)
  assert(tween:get() == 8 and not tween:is_moving())
end)

test("a tween reads part way between two ticks", function()
  local tween = trx.ui.tween(0):to(10, 10)
  ticks(3)
  fake.set_rate(0.5)
  -- Drawn half way between the second tick and the third.
  assert(tween:get() == 2.5, tween:get())
  fake.set_rate(1)
end)

test("a tween turned mid-way sets off from where it was", function()
  fake.set_rate(1)
  local tween = trx.ui.tween(0):to(10, 10)
  ticks(5)
  tween:to(0, 5)
  assert(tween:get() == 5, tween:get())
  ticks(5)
  assert(tween:get() == 0)
end)

test("a delayed tween holds still until it starts", function()
  fake.set_rate(1)
  local tween = trx.ui.tween(0):to(8, 4, { delay = 3 })
  ticks(2)
  assert(tween:get() == 0 and tween:is_moving(), tween:get())
  ticks(3)
  assert(tween:get() == 4, tween:get())
  ticks(2)
  assert(tween:get() == 8 and not tween:is_moving())
end)

test("a tween follows the curve it is given", function()
  fake.set_rate(1)
  local tween = trx.ui.tween(0):to(8, 4, { curve = trx.ui.ease.quad_in })
  ticks(2)
  assert(tween:get() == 2, tween:get())
  ticks(2)
  assert(tween:get() == 8)
end)

test("a tween refuses a curve that is not one of trx.ui.ease", function()
  local ok = pcall(function()
    trx.ui.tween(0):to(8, 4, {
      curve = function(ratio)
        return ratio
      end,
    })
  end)
  assert(not ok, "the curve was taken")
end)

test("a tween set outright stops where it is put", function()
  local tween = trx.ui.tween(0):to(10, 10)
  tween:set(7)
  assert(tween:get() == 7 and not tween:is_moving())
end)

-- A carousel of three models, ten units wide each, a hundred apart.
local function carousel(selected, count)
  local items = {}
  for i = 1, count or 3 do
    items[i] = i
  end
  return widgets.Carousel({
    items = items,
    selected = selected,
    build = function()
      return widgets.Model({ object = 5, w = 10, h = 10 })
    end,
    w = 400,
    h = 10,
    spacing = 100,
  })
end

-- Each model a carousel draws, by where its middle stands across.
local function row_of(row)
  local result = {}
  for _, op in
    ipairs(fake.paint(function()
      row:paint(0, 0, 400, 10)
    end))
  do
    local x, w = op:match("^model .- x=(%-?%d+) y=%-?%d+ w=(%d+) ")
    if x ~= nil then
      result[#result + 1] = { middle = x + w / 2, w = tonumber(w) }
    end
  end
  table.sort(result, function(a, b)
    return a.middle < b.middle
  end)
  return result
end

-- A length on the canvas as the fake records it, in screen pixels.
local function px(length)
  return primitive.to_screen(length)
end

local function near(a, b)
  return math.abs(a - b) <= 1
end

test("a carousel draws the selected item in the middle at its size", function()
  fake.set_rate(1)
  local row = carousel(trx.signal.new(1))
  local drawn = row_of(row)
  assert(#drawn == 3, #drawn)
  assert(near(drawn[2].middle, px(200)), drawn[2].middle)
  assert(near(drawn[2].w, px(10)), drawn[2].w)
  assert(near(drawn[1].middle, px(100)) and drawn[1].w < drawn[2].w)
  assert(near(drawn[3].middle, px(300)) and drawn[3].w < drawn[2].w)
end)

test("a carousel slides to what is selected", function()
  fake.set_rate(1)
  local selected = trx.signal.new(1)
  local row = carousel(selected)
  selected:set(2)
  assert(row.is_sliding(), "the row did not start sliding")
  ticks(8)
  assert(not row.is_sliding(), "the row is still sliding")
  local drawn = row_of(row)
  assert(near(drawn[2].middle, px(200)) and near(drawn[2].w, px(10)))
end)

test("a carousel reads part way through a slide", function()
  fake.set_rate(1)
  local selected = trx.signal.new(1)
  local row = carousel(selected)
  selected:set(2)
  ticks(4)
  local middle = row_of(row)
  -- Half way, the two items stand either side of the middle.
  assert(not near(middle[2].middle, px(200)) or middle[2].w < px(10))
  ticks(4)
end)

test("a carousel wraps the short way round", function()
  fake.set_rate(1)
  local selected = trx.signal.new(1)
  local row = carousel(selected)
  selected:set(3)
  ticks(2)
  -- Going from the first to the last of three is one place to the left, so
  -- the row moves right rather than two places left.
  local drawn = row_of(row)
  local moved_right = false
  for _, item in ipairs(drawn) do
    if item.middle > px(200) + 1 and item.middle < px(300) - 1 then
      moved_right = true
    end
  end
  assert(moved_right, "the row went the long way")
  ticks(6)
end)

test("a carousel keeps items beyond the neighbours out of sight", function()
  fake.set_rate(1)
  local row = carousel(trx.signal.new(1), 5)
  assert(#row_of(row) == 3, #row_of(row))
end)

return h.report()
