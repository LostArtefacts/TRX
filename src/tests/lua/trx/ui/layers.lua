-- The layer stack: where layers draw, which one reads input, and how they
-- close.

local h = require("harness")
local test = h.test

local Role = trx.input.Role

local function label(text)
  return trx.ui.widgets.Label({ text = text })
end

-- The line a scene drew for a text, or nil.
local function drawn(ops, text)
  for _, op in ipairs(ops) do
    local suffix = " text=" .. text
    if op:match("^text ") and op:sub(-#suffix) == suffix then
      return op
    end
  end
  return nil
end

-- Where a text was drawn in a scene, as the position of its draw and the
-- layer it was drawn in.
local function placement(ops, layers, text)
  local suffix = " text=" .. text
  for i, op in ipairs(ops) do
    if op:match("^text ") and op:sub(-#suffix) == suffix then
      return i, layers[i]
    end
  end
  return nil
end

local function xy_of(op)
  local x, y = op:match(" x=(%-?%d+) y=(%-?%d+)")
  return tonumber(x), tonumber(y)
end

local function push(settings)
  settings.root = settings.root or label("layer")
  return trx.ui.layers.push(settings)
end

-- Every case leaves the stack empty, so a case that forgets to close a layer
-- fails the next one rather than passing it by accident.
local function fresh()
  assert(trx.ui.layers.count() == 0, "a layer was left open by another case")
end

test("an input hint shows the keys of each role before its words", function()
  assert(
    trx.ui.input_hint_text(Role.MENU_CONFIRM, "Use")
      == "\\{input menu_confirm} Use"
  )
  assert(
    trx.ui.input_hint_text({ Role.STEP_LEFT, Role.STEP_RIGHT })
      == "\\{input step_left}\\{input step_right}"
  )
end)

test("an input hint for no such role raises", function()
  h.raises(function()
    trx.ui.input_hint_text(-1, "Use")
  end, "unknown input role")
end)

test("an input hint widget draws the keys and what they do", function()
  fresh()
  local layer = push({
    root = trx.ui.widgets.InputHint({
      input = Role.MENU_BACK,
      text = "Cancel",
    }),
  })
  local ops = fake.scene()
  layer:close()
  assert(drawn(ops, "\\{input menu_back} Cancel"), "the hint was not drawn")
end)

test("a layer draws over a widget placed in a region", function()
  fresh()
  local placed = label("placed")
  trx.ui.regions.place(trx.ui.Region.CENTER, placed)
  local layer = push({ root = label("layer") })

  local ops, layers = fake.scene(), fake.scene_layers()
  layer:close()
  trx.ui.regions.remove(placed)

  local over_at, over_layer = placement(ops, layers, "layer")
  local under_at, under_layer = placement(ops, layers, "placed")
  assert(over_layer == under_layer, "the two are in different layers")
  assert(over_at > under_at, "the layer drew before the widget")
end)

local function count_drawn(text)
  local count = 0
  for _, op in ipairs(fake.scene()) do
    if op:match("^text ") and op:sub(-#text - 6) == " text=" .. text then
      count = count + 1
    end
  end
  return count
end

test("a widget placed twice moves rather than draws twice", function()
  local widget = label("twice")
  trx.ui.regions.place(trx.ui.Region.TOP_LEFT, widget)
  trx.ui.regions.place(trx.ui.Region.TOP_RIGHT, widget)
  assert(count_drawn("twice") == 1, "drawn " .. count_drawn("twice"))
  assert(trx.ui.regions.remove(widget))
  assert(not trx.ui.regions.remove(widget), "it was in a second region")
end)

test("a widget placed again stops following its old region", function()
  local where = trx.signal.new(trx.ui.Region.TOP_LEFT)
  local widget = label("moved")
  trx.ui.regions.place(where, widget)
  trx.ui.regions.place(trx.ui.Region.BOTTOM_LEFT, widget)
  where:set(trx.ui.Region.TOP_RIGHT)
  assert(count_drawn("moved") == 1, "drawn " .. count_drawn("moved"))
  assert(trx.ui.regions.remove(widget))
  assert(not trx.ui.regions.remove(widget), "it was in a second region")
end)

test("a widget in another tree cannot be placed", function()
  local body = label("inside")
  local outer = trx.ui.widgets.Pad({ child = body })
  h.raises(function()
    trx.ui.regions.place(trx.ui.Region.TOP_LEFT, body)
  end, "already part of another widget")
  assert(not trx.ui.regions.remove(body), "a tree child was in a region")
  outer:release()
end)

test("a layer's root cannot be used anywhere else", function()
  fresh()
  local root = label("root")
  local layer = push({ root = root })
  h.raises(function()
    trx.ui.regions.place(trx.ui.Region.TOP_LEFT, root)
  end, "already in use")
  h.raises(function()
    push({ root = root })
  end, "already in use")
  h.raises(function()
    trx.ui.widgets.Pad({ child = root })
  end, "already in use")
  layer:close()
  h.raises(function()
    trx.ui.regions.place(trx.ui.Region.TOP_LEFT, root)
  end, "the widget was released")
end)

test("a placed widget cannot be a layer's root", function()
  fresh()
  local placed = label("placed")
  trx.ui.regions.place(trx.ui.Region.TOP_LEFT, placed)
  h.raises(function()
    push({ root = placed })
  end, "already in use")
  trx.ui.regions.remove(placed)
end)

test("releasing a placed widget removes it from its region", function()
  local widget = label("released")
  trx.ui.regions.place(trx.ui.Region.TOP_LEFT, widget)
  widget:release()
  assert(count_drawn("released") == 0, "the released widget was drawn")
  assert(not trx.ui.regions.remove(widget), "it was still in its region")
end)

test("a widget that a level script removed is not removed again", function()
  local widget = label("kept")
  fake.as_level_script(function()
    trx.ui.regions.place(trx.ui.Region.TOP_LEFT, widget)
    trx.ui.regions.remove(widget)
  end)
  trx.ui.regions.place(trx.ui.Region.TOP_LEFT, widget)
  fake.end_level()
  local drawn_after = count_drawn("kept")
  trx.ui.regions.remove(widget)
  assert(drawn_after == 1, "the level end removed the widget")
end)

test("a layer draws behind the engine interface", function()
  fresh()
  local layer = push({ root = label("layer") })
  local ops, layers = fake.scene(), fake.scene_layers()
  layer:close()
  local _, drawn_in = placement(ops, layers, "layer")
  assert(drawn_in == "under", tostring(drawn_in))
end)

test("a later layer draws over an earlier one", function()
  fresh()
  local lower = push({ root = label("lower") })
  local upper = push({ root = label("upper") })

  local ops, layers = fake.scene(), fake.scene_layers()
  upper:close()
  lower:close()

  local upper_at = placement(ops, layers, "upper")
  local lower_at = placement(ops, layers, "lower")
  assert(upper_at > lower_at, "the later layer drew first")
end)

test("a layer is centered in the safe area by default", function()
  fresh()
  local root = label("centered")
  local layer = push({ root = root })
  local ops = fake.scene()
  local w, height = root:measure()
  layer:close()

  local safe = trx.ui.safe_area
  local x, y = xy_of(drawn(ops, "centered"))
  assert(x == math.floor(safe.x + (safe.width - w) / 2))
  assert(y == math.floor(safe.y + (safe.height - height) / 2))
end)

test("a layer too large for the safe area starts at its top left", function()
  fresh()
  local at
  local layer = push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return 100000, 100000
      end,
      paint = function(x, y)
        at = { x, y }
      end,
    }),
  })
  fake.scene()
  layer:close()
  local safe = trx.ui.safe_area_for(100000)
  assert(at ~= nil, "the layer was not drawn")
  assert(at[1] == safe.x and at[2] == safe.y)
end)

test("a layer can say where it goes", function()
  fresh()
  local layer = push({
    root = label("placed"),
    place = function()
      return 12, 34
    end,
  })
  local ops = fake.scene()
  layer:close()
  local x, y = xy_of(drawn(ops, "placed"))
  assert(x == 12 and y == 34)
end)

test("a layer stands where its placement puts it", function()
  fresh()
  local root = label("placed")
  local layer = push({ root = root, place = { x = 0, y = 1, margin = 10 } })
  local ops = fake.scene()
  local w, height = root:measure()
  layer:close()

  local safe = trx.ui.safe_area_for(w)
  local x, y = xy_of(drawn(ops, "placed"))
  assert(x == math.floor(safe.x + 10), x)
  assert(y == math.floor(safe.y + safe.height - 10 - height), y)
end)

test("a layer can stand on the whole canvas", function()
  fresh()
  local root = label("placed")
  local layer = push({ root = root, place = { canvas = true, x = 1, y = 0 } })
  local ops = fake.scene()
  local w = root:measure()
  layer:close()

  local x, y = xy_of(drawn(ops, "placed"))
  assert(x == math.floor(trx.ui.canvas.width - w), x)
  assert(y == 0, y)
end)

test("a layer works its corner out once while nothing changes", function()
  fresh()
  local calls = 0
  local layer = push({
    place = function()
      calls = calls + 1
      return 12, 34
    end,
  })
  fake.scene()
  fake.scene()
  fake.scene()
  layer:close()
  assert(calls == 1, calls)
end)

test("a layer moves when a signal its placement reads changes", function()
  fresh()
  local share = trx.signal.new(0)
  local layer = push({ root = label("placed"), place = { y = share } })
  local _, top = xy_of(drawn(fake.scene(), "placed"))
  share:set(1)
  local _, bottom = xy_of(drawn(fake.scene(), "placed"))
  layer:close()
  assert(bottom > top, ("%d is not below %d"):format(bottom, top))
end)

test("a new root can take a new placement", function()
  fresh()
  local layer = push({ place = { x = 0, y = 0 } })
  layer:set_root(label("moved"), function()
    return 12, 34
  end)
  local ops = fake.scene()
  layer:close()
  local x, y = xy_of(drawn(ops, "moved"))
  assert(x == 12 and y == 34)
end)

test("a layer reads no input on the tick it opens", function()
  fresh()
  local reads = 0
  local layer = push({
    on_input = function()
      reads = reads + 1
    end,
  })
  fake.tick()
  assert(reads == 0)
  fake.tick()
  assert(reads == 1)
  layer:close()
end)

test("only the top layer reads input", function()
  fresh()
  local lower_reads, upper_reads = 0, 0
  local lower = push({
    on_input = function()
      lower_reads = lower_reads + 1
    end,
  })
  local upper = push({
    on_input = function()
      upper_reads = upper_reads + 1
    end,
  })
  fake.tick()
  fake.tick()
  upper:close()
  lower:close()
  assert(upper_reads == 1)
  assert(lower_reads == 0)
end)

test("a layer that takes no input leaves it to the one below", function()
  fresh()
  local reads = 0
  local lower = push({
    on_input = function()
      reads = reads + 1
    end,
  })
  local upper = push({ modal = false })
  fake.tick()
  fake.tick()
  assert(trx.ui.layers.active() == lower)
  upper:close()
  lower:close()
  assert(reads == 1)
end)

test("a press is used up by the layer that reads it", function()
  fresh()
  local first, second
  local layer = push({
    on_input = function(_, keys)
      first = keys:pressed(Role.MENU_CONFIRM)
      second = keys:pressed(Role.MENU_CONFIRM)
    end,
  })
  fake.tick()
  fake.press(Role.MENU_CONFIRM)
  fake.tick()
  layer:close()
  assert(first == true)
  assert(second == false)
  assert(fake.held_off(Role.MENU_CONFIRM))
end)

test("a held menu key keeps reading while the layer stays open", function()
  fresh()
  local reads = 0
  local layer = push({
    on_input = function(_, keys)
      if keys:pressed(Role.MENU_DOWN) then
        reads = reads + 1
      end
    end,
  })
  fake.tick()
  -- The input reports a held menu key as pressed again on each repeat, and the
  -- fake keeps reporting it pressed.
  fake.press(Role.MENU_DOWN)
  fake.tick()
  fake.tick()
  fake.tick()
  assert(not fake.held_off(Role.MENU_DOWN), "reading held the key off")
  layer:close()
  assert(reads == 3, reads)
end)

test("a press that closes a layer does not reach the layer below", function()
  fresh()
  local lower_saw = false
  local lower = push({
    on_input = function(_, keys)
      lower_saw = lower_saw or keys:pressed(Role.MENU_BACK)
    end,
  })
  push({
    on_input = function(layer, keys)
      if keys:pressed(Role.MENU_BACK) then
        layer:close()
      end
    end,
  })
  fake.tick()
  fake.press(Role.MENU_BACK)
  fake.tick()
  fake.tick()
  lower:close()
  assert(not lower_saw)
end)

test("a held role counts the ticks it is held", function()
  fresh()
  local count
  local layer = push({
    on_input = function(_, keys)
      count = keys:held_for(Role.UNBIND_KEY)
    end,
  })
  fake.tick()
  fake.hold(Role.UNBIND_KEY, true)
  fake.tick()
  fake.tick()
  fake.tick()
  assert(count == 3)
  fake.hold(Role.UNBIND_KEY, false)
  fake.tick()
  assert(count == 0)
  layer:close()
end)

test("a layer closes once", function()
  fresh()
  local closes = 0
  local layer = push({
    on_close = function()
      closes = closes + 1
    end,
  })
  assert(layer.is_open)
  assert(layer:close() == true)
  assert(layer:close() == false)
  assert(not layer.is_open)
  assert(closes == 1)
end)

test("an error while reading input closes the layer", function()
  fresh()
  local closed = false
  local layer = push({
    on_input = function()
      error("broken")
    end,
    on_close = function()
      closed = true
    end,
  })
  fake.tick()
  fake.tick()
  assert(not layer.is_open)
  assert(closed)
  assert(fake.errors() == 1)
end)

test("an error while reading input reports a close error too", function()
  fresh()
  push({
    on_input = function()
      error("input broke")
    end,
    on_close = function()
      error("close broke")
    end,
  })
  fake.tick()
  fake.tick()
  local message = fake.last_error()
  assert(message:find("input broke", 1, true), message)
  assert(message:find("close broke", 1, true), message)
end)

test("an error while drawing closes the layer", function()
  fresh()
  local broken = trx.ui.widgets.Label({ text = "x" })
  broken.on_paint = function()
    error("broken")
  end
  local layer = push({ root = broken })
  fake.scene()
  assert(not layer.is_open)
  assert(fake.errors() == 1)
end)

test("a level script's layer closes with the level", function()
  fresh()
  local layer
  fake.as_level_script(function()
    layer = push({})
  end)
  fake.end_level()
  assert(not layer.is_open)
end)

test("a global script's layer stays across a level", function()
  fresh()
  local layer = push({})
  fake.end_level()
  assert(layer.is_open)
  layer:close()
end)

test("a layer draws the root it was last given", function()
  fresh()
  local layer = push({ root = label("old") })
  layer:set_root(label("new"))
  local ops = fake.scene()
  layer:close()
  assert(drawn(ops, "old") == nil)
  assert(drawn(ops, "new") ~= nil)
end)

test("the stack holds a limited number of layers", function()
  fresh()
  local layers = {}
  h.raises(function()
    for i = 1, 100 do
      layers[i] = push({})
    end
  end, "too many layers")
  for _, layer in ipairs(layers) do
    layer:close()
  end
end)

-------------------------------------------------------------------------------
-- The list
-------------------------------------------------------------------------------

local function list(settings)
  return trx.ui.widgets.List(settings)
end

local function rows(...)
  local result = {}
  for i, text in ipairs({ ... }) do
    result[i] = { text = text }
  end
  return result
end

local function texts(ops)
  local result = {}
  for _, op in ipairs(ops) do
    local text = op:match("^text .- text=(.*)$")
    if text ~= nil then
      result[#result + 1] = text
    end
  end
  return result
end

test("a list draws only the rows in view", function()
  fresh()
  local l = list({ rows = rows("r1", "r2", "r3", "r4", "r5"), page_rows = 2 })
  l:select(4)
  local layer = push({ root = l })
  local drawn_texts = texts(fake.scene())
  layer:close()
  assert(#drawn_texts == 4, table.concat(drawn_texts, ","))
  assert(drawn_texts[1] == "\\{arrow up}")
  assert(drawn_texts[2] == "r3")
  assert(drawn_texts[3] == "r4")
  assert(drawn_texts[4] == "\\{arrow down}")
end)

test("a list keeps the cursor in view when it shows fewer rows", function()
  fresh()
  local page_rows = trx.signal.new(5)
  local l =
    list({ rows = rows("r1", "r2", "r3", "r4", "r5"), page_rows = page_rows })
  l:select(5)
  page_rows:set(2)
  local layer = push({ root = l })
  local drawn_texts = texts(fake.scene())
  layer:close()
  assert(#drawn_texts == 3, table.concat(drawn_texts, ","))
  assert(drawn_texts[1] == "\\{arrow up}")
  assert(drawn_texts[2] == "r4")
  assert(drawn_texts[3] == "r5")
end)

test("a list answers the row the player picks", function()
  fresh()
  local l = list({ rows = rows("a", "b", "c") })
  local picked
  local layer = push({
    root = l,
    on_input = function(_, keys)
      picked = l:control(keys) or picked
    end,
  })
  fake.tick()
  fake.press(trx.input.Role.MENU_DOWN)
  fake.tick()
  fake.release_all()
  fake.press(trx.input.Role.MENU_CONFIRM)
  fake.tick()
  layer:close()
  assert(picked == 2, tostring(picked))
end)

test("a list reports the back key", function()
  fresh()
  local l = list({ rows = rows("a") })
  local backed = false
  local layer = push({
    root = l,
    on_input = function(_, keys)
      local _, cancelled = l:control(keys)
      backed = backed or cancelled
    end,
  })
  fake.tick()
  fake.press(trx.input.Role.MENU_BACK)
  fake.tick()
  layer:close()
  assert(backed)
end)

-------------------------------------------------------------------------------
-- The sleek bar
-------------------------------------------------------------------------------

local function quads(ops)
  local result = {}
  for _, op in ipairs(ops) do
    local x, y, w, h, color = op:match(
      "^quad x=(%-?%d+) y=(%-?%d+) z=%-?%d+ w=(%-?%d+) h=(%-?%d+) color=(%x+)$"
    )
    if x ~= nil then
      result[#result + 1] = {
        x = tonumber(x),
        y = tonumber(y),
        w = tonumber(w),
        h = tonumber(h),
        color = color,
      }
    end
  end
  return result
end

local function bar_scene(progress)
  local layer = push({
    root = trx.ui.widgets.Resize({
      w = 100,
      child = trx.ui.widgets.SleekBar({ progress = progress }),
    }),
    place = function()
      return 10, 20
    end,
  })
  local drawn = quads(fake.scene())
  layer:close()
  return drawn
end

test("a sleek bar is four units tall and as wide as its box", function()
  local w, height = trx.ui.widgets.SleekBar({ progress = 0 }):measure()
  assert(w == 0, w)
  assert(height == 4, height)
end)

test("a sleek bar fills inside a dark frame", function()
  fresh()
  local drawn = bar_scene(0.5)
  assert(#drawn == 2, #drawn)
  assert(drawn[1].color == "060606ff")
  assert(drawn[1].x == 10 and drawn[1].y == 20)
  assert(drawn[1].w == 100 and drawn[1].h == 4)
  assert(drawn[2].x == 11 and drawn[2].y == 21)
  assert(drawn[2].w == 49 and drawn[2].h == 2)
end)

test("a sleek bar fills in the game's colour", function()
  fresh()
  local drawn = bar_scene(1)
  assert(drawn[2].color == "a1833cff", drawn[2].color)
end)

test("a sleek bar's frame is as thick on every side", function()
  for _, size in ipairs({ { 1000, 750 }, { 1366, 768 }, { 1700, 1275 } }) do
    fake.set_viewport(size[1], size[2])
    for _, y in ipairs({ 20, 20.3, 20.6, 21.1 }) do
      fresh()
      local layer = push({
        root = trx.ui.widgets.Resize({
          w = 100,
          child = trx.ui.widgets.SleekBar({ progress = 1 }),
        }),
        place = function()
          return 10.4, y
        end,
      })
      local drawn = quads(fake.scene())
      layer:close()
      local frame, fill = drawn[1], drawn[2]
      local top = fill.y - frame.y
      local bottom = frame.y + frame.h - fill.y - fill.h
      local left = fill.x - frame.x
      local right = frame.x + frame.w - fill.x - fill.w
      local where = ("%dx%d at y=%s"):format(size[1], size[2], y)
      assert(top >= 1, where)
      assert(top == bottom, where .. ": top and bottom differ")
      assert(left == right, where .. ": left and right differ")
      assert(top == left, where .. ": sides and ends differ")
    end
  end
  fake.set_viewport(640, 480)
end)

test("a sleek bar keeps its fill within its frame", function()
  fresh()
  assert(bar_scene(2)[2].w == 98)
  assert(#bar_scene(-1) == 1, "an empty bar drew a fill")
end)

local BLACK = trx.math.color("000000")

local function fade_ticks(count)
  for _ = 1, count do
    fake.tick()
  end
end

test("a fade layer needs something to hold", function()
  h.raises(function()
    trx.ui.layers.fade({})
  end, "a fade layer needs something to hold")
end)

test("a fade layer closes at rest when told to, and says so once", function()
  local done, closed = 0, 0
  local layer = trx.ui.layers.fade({
    fill = BLACK,
    on_close = function()
      closed = closed + 1
    end,
  })
  layer:to(0, 2, {
    close = true,
    on_done = function()
      done = done + 1
    end,
  })
  assert(layer.is_open)
  fade_ticks(4)
  assert(not layer.is_open, "the layer stayed open")
  assert(done == 1, "the move was reported " .. done .. " times")
  assert(closed == 1, "the close was reported " .. closed .. " times")
  assert(not layer:close(), "a closed layer closed again")
end)

test("an error as a fade layer closes still ends its move", function()
  local errors = fake.errors()
  local done = 0
  local layer = trx.ui.layers.fade({
    fill = BLACK,
    on_close = function()
      error("broken")
    end,
  })
  layer:to(0, 2, {
    close = true,
    on_done = function()
      done = done + 1
    end,
  })
  fade_ticks(4)
  assert(not layer.is_open, "the layer stayed open")
  assert(done == 1, "the move was reported " .. done .. " times")
  assert(fake.errors() == errors + 1, "the error was not raised")
end)

test("an error as a fade move ends leaves the layer whole", function()
  local errors = fake.errors()
  local layer = trx.ui.layers.fade({ fill = BLACK, opacity = 0 })
  layer:to(1, 1, {
    on_done = function()
      error("broken")
    end,
  })
  fade_ticks(3)
  assert(layer.is_open, "the layer closed")
  assert(fake.errors() == errors + 1, "the error was not raised")
  assert(layer:close())
end)

test("a move cut short by another reports only the last", function()
  local first, second = 0, 0
  local layer = trx.ui.layers.fade({ fill = BLACK, opacity = 0 })
  layer:to(1, 8, {
    on_done = function()
      first = first + 1
    end,
  })
  fake.tick()
  layer:to(0, 1, {
    on_done = function()
      second = second + 1
    end,
  })
  fade_ticks(10)
  assert(first == 0 and second == 1, "first=" .. first .. " second=" .. second)
  assert(layer.is_open, "a move without close closed the layer")
  layer:close()
end)

test("a move of no ticks ends on the next tick", function()
  local done = 0
  local layer = trx.ui.layers.fade({ fill = BLACK, opacity = 0 })
  layer:to(1, 0, {
    on_done = function()
      done = done + 1
    end,
  })
  assert(done == 0, "the move ended inside to()")
  assert(layer.opacity == 1)
  fake.tick()
  assert(done == 1, "the move did not end on the next tick")
  layer:close()
end)

test("a move takes its ticks whatever the fade setting", function()
  fake.set_fade_effects(false)
  local layer = trx.ui.layers.fade({ fill = BLACK, opacity = 0 })
  layer:to(1, 8)
  fake.tick()
  assert(layer.opacity < 1, "the move ended at once")
  layer:close()
  fake.set_fade_effects(true)
end)

test("fade layers paint in the fade layer, in the order made", function()
  local first = trx.ui.layers.fade({ fill = BLACK, opacity = 0.5 })
  local second = trx.ui.layers.fade({ fill = trx.math.color("ffffff") })
  local ops, layers = fake.scene(), fake.scene_layers()
  first:close()
  second:close()
  local quads = {}
  for i, op in ipairs(ops) do
    if op:match("^quad") and layers[i] == "fade" then
      quads[#quads + 1] = op
    end
  end
  assert(#quads == 2, "painted " .. #quads .. " fades")
  assert(quads[1]:find("color=00000080", 1, true), quads[1])
  assert(quads[2]:find("color=ffffffff", 1, true), quads[2])
end)

test("a closed fade layer paints nothing", function()
  trx.ui.layers.fade({ fill = BLACK }):close()
  for i, op in ipairs(fake.scene()) do
    assert(fake.scene_layers()[i] ~= "fade", op)
  end
end)

test("a snapshot fade layer opens and closes", function()
  local layer = trx.ui.layers.fade({ snapshot = true })
  assert(layer.is_open and layer.opacity == 1)
  assert(layer:close())
end)

test("a backdrop fade layer opens and closes", function()
  local layer = trx.ui.layers.fade({ backdrop = "transparent", opacity = 0 })
  assert(layer.is_open and layer.opacity == 0)
  assert(layer:close())
  h.raises(function()
    trx.ui.layers.fade({ backdrop = "glitter" })
  end, "unknown backdrop")
end)

test("a level script's fade layer closes with the level", function()
  local layer
  fake.as_level_script(function()
    layer = trx.ui.layers.fade({ fill = BLACK })
  end)
  fake.end_level()
  assert(not layer.is_open)
end)

test("a layer releases the root that a new one replaces", function()
  fresh()
  local old = label("old")
  local layer = push({ root = old })
  layer:set_root(label("new"))
  layer:close()
  h.raises(function()
    trx.ui.regions.place(trx.ui.Region.TOP_LEFT, old)
  end, "the widget was released")
end)

test("a widget taken out of its region can be placed again", function()
  local widget = label("again")
  trx.ui.regions.place(trx.ui.Region.TOP_LEFT, widget)
  assert(trx.ui.regions.remove(widget))
  assert(count_drawn("again") == 0, "the removed widget was drawn")
  trx.ui.regions.place(trx.ui.Region.TOP_RIGHT, widget)
  assert(count_drawn("again") == 1, "the widget did not come back")
  widget:release()
end)

test("a level script's placed widget is released with the level", function()
  local widget
  fake.as_level_script(function()
    widget = label("level widget")
    trx.ui.regions.place(trx.ui.Region.TOP_LEFT, widget)
  end)
  fake.end_level()
  assert(count_drawn("level widget") == 0, "the widget outlived the level")
  h.raises(function()
    trx.ui.regions.place(trx.ui.Region.TOP_LEFT, widget)
  end, "the widget was released")
end)

local function counting(role)
  local counter = { reads = 0 }
  counter.layer = push({
    on_input = function(_, keys)
      if keys:pressed(role) then
        counter.reads = counter.reads + 1
      end
    end,
  })
  return counter
end

local function ticks(count)
  for _ = 1, count do
    fake.tick()
  end
end

test("a layer without an input callback reads no input", function()
  fresh()
  local below = push({ on_input = function() end })
  local above = push({})
  assert(trx.ui.layers.active() == below, "the layer above took the input")
  above:close()
  below:close()
end)

test("a modal layer holds the input back without reading it", function()
  fresh()
  local below = push({ on_input = function() end })
  local above = push({ modal = true })
  assert(trx.ui.layers.active() == above, "the layer below kept the input")
  above:close()
  below:close()
end)

test("during play a layer that reads input takes the devices", function()
  fresh()
  fake.set_playing(true)
  local layer = push({ on_input = function() end })
  assert(trx.input.is_grabbed(), "the game still has the devices")
  layer:close()
  assert(not trx.input.is_grabbed(), "the game did not get the devices back")
end)

test("during play a layer that reads no input leaves the devices", function()
  fresh()
  fake.set_playing(true)
  local layer = push({})
  assert(not trx.input.is_grabbed(), "the layer took the devices")
  layer:close()
end)

test("outside play a layer leaves the devices to the game", function()
  fresh()
  local layer = push({ on_input = function() end })
  assert(not trx.input.is_grabbed(), "the layer took the devices")
  layer:close()
end)

test("during play a press reads once while the key is held", function()
  fresh()
  fake.set_playing(true)
  local counter = counting(Role.MENU_CONFIRM)
  fake.tick()
  fake.press(Role.MENU_CONFIRM)
  ticks(30)
  counter.layer:close()
  fake.release_all()
  assert(counter.reads == 1, counter.reads)
end)

test("during play a key held from before the layer is no press", function()
  fresh()
  fake.set_playing(true)
  fake.press(Role.MENU_CONFIRM)
  local counter = counting(Role.MENU_CONFIRM)
  ticks(3)
  assert(counter.reads == 0, "the held key read as a press")
  fake.release_all()
  fake.tick()
  fake.press(Role.MENU_CONFIRM)
  fake.tick()
  counter.layer:close()
  fake.release_all()
  assert(counter.reads == 1, counter.reads)
end)

-- The ticks, counted from the press, on which a layer read a key that the
-- player held for the given number of ticks during play.
local function presses_of(role, held)
  fresh()
  fake.set_playing(true)
  local at = {}
  local count = 0
  local layer = push({
    on_input = function(_, keys)
      count = count + 1
      if keys:pressed(role) then
        at[#at + 1] = count
      end
    end,
  })
  fake.tick()
  fake.tick()
  count = 0
  fake.press(role)
  ticks(held)
  layer:close()
  fake.release_all()
  return table.concat(at, ",")
end

test("during play up and down repeat as the game's menus do", function()
  -- Once on the press, then from the 0.4 s delay every 0.1 s.
  assert(presses_of(Role.MENU_UP, 24) == "1,15,18,21,24")
  assert(presses_of(Role.MENU_DOWN, 24) == "1,15,18,21,24")
end)

test("during play left and right repeat as the game's menus do", function()
  -- Once on the press, then from the 0.4 s delay every 0.2 s.
  assert(presses_of(Role.MENU_LEFT, 24) == "1,18,24")
  assert(presses_of(Role.MENU_RIGHT, 24) == "1,18,24")
end)

test("during play a held confirm key does not repeat", function()
  assert(presses_of(Role.MENU_CONFIRM, 24) == "1")
end)

test(
  "during play a level's layer lets the devices go with the level",
  function()
    fresh()
    fake.set_playing(true)
    fake.as_level_script(function()
      push({ on_input = function() end })
    end)
    assert(trx.input.is_grabbed())
    fake.end_level()
    assert(not trx.input.is_grabbed(), "the game did not get the devices back")
  end
)

return h.report()
