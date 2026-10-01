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

local function z_of(op)
  return tonumber(op:match(" z=(%-?%d+)"))
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

test("a layer draws over a widget placed in a region", function()
  fresh()
  local placed = label("placed")
  trx.ui.regions.place(trx.ui.Region.CENTER, placed)
  local layer = push({ root = label("layer") })

  local ops = fake.scene()
  layer:close()
  trx.ui.regions.remove(placed)

  assert(z_of(drawn(ops, "layer")) < z_of(drawn(ops, "placed")))
end)

test("a layer draws behind the engine interface", function()
  fresh()
  local layer = push({ root = label("layer") })
  local ops = fake.scene()
  layer:close()
  assert(z_of(drawn(ops, "layer")) > 0)
end)

test("a later layer draws over an earlier one", function()
  fresh()
  local lower = push({ root = label("lower") })
  local upper = push({ root = label("upper") })

  local ops = fake.scene()
  upper:close()
  lower:close()

  assert(z_of(drawn(ops, "upper")) < z_of(drawn(ops, "lower")))
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

test("a layer can take room in a region", function()
  fresh()
  local layer = push({
    root = label("in-region"),
    region = trx.ui.Region.BOTTOM_CENTER,
  })
  local ops = fake.scene()
  layer:close()
  assert(drawn(ops, "in-region") ~= nil)
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
  assert(trx.ui.layers.top() == lower)
  assert(not upper:is_top())
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
  local l = list({ rows = rows("r1", "r2", "r3", "r4", "r5"), visible = 2 })
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
  local visible = trx.signal.new(5)
  local l =
    list({ rows = rows("r1", "r2", "r3", "r4", "r5"), visible = visible })
  l:select(5)
  visible:set(2)
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

test("a list leaves the back key to its layer", function()
  fresh()
  local l = list({ rows = rows("a") })
  local backed = false
  local layer = push({
    root = l,
    on_input = function(_, keys)
      l:control(keys)
      backed = backed or keys:pressed(trx.input.Role.MENU_BACK)
    end,
  })
  fake.tick()
  fake.press(trx.input.Role.MENU_BACK)
  fake.tick()
  layer:close()
  assert(backed)
end)

return h.report()
