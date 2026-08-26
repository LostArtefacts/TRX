-- The shipped assault module: what each readout spells, in which colors, and
-- which of them the module gives the top middle to.
--
-- Every field, enum spelling and format the module names is unchecked until
-- something calls it, so the whole point here is to draw each of them.

local h = require("harness")
local test = h.test

local QUAD = trx.assault.Track.QUAD
local COURSE = trx.assault.Track.COURSE

-- The palettes the engine draws these in. The fake reports TR3, which shades
-- the digits; TR2 draws the same characters flat white.
local NEUTRAL = { top = "ffffffff", bottom = "404040ff" }
local PINK = { top = "ff00ffff", bottom = "400040ff" }
local GREY = { top = "808080ff", bottom = "1a1a1aff" }
local GREEN = { top = "59bf33ff", bottom = "1a4000ff" }
local RED = { top = "e63300ff", bottom = "4d0000ff" }

-- The character each sprite of the assault course object stands for.
local GLYPHS = "0123456789:.Ts"

-- Two characters of one readout sit within a few units of each other, and one
-- readout stands a whole digit clear of the next. A space draws no sprite, so
-- what a readout spells here runs the characters together.
local ROW_TOLERANCE = 8

-- The readouts are polled, so a change reaches a widget on the next tick.
local function draw_after(setup)
  -- Every size the widgets measure is a multiple of the text scale, and a
  -- reset leaves the settings at zero.
  trx.config.set("ui.text_scale", 1)
  fake.set_playing(true)
  fake.set_lap(0, 0)
  fake.set_run(COURSE, 0, 0, 0, 0)
  setup()
  fake.tick()
  local description, balanced = fake.draw_regions()
  assert(balanced, "a widget was left open for the next frame to draw into")
  assert(
    fake.errors() == 0,
    ("%d region(s) raised - see the errors above"):format(fake.errors())
  )
  return description
end

local function sprites(description)
  local drawn = {}
  for idx, x, y, top, bottom in
    description:gmatch(
      "sprite idx=(%d+) x=(%-?%d+) y=(%-?%d+)[^\n]-tl=(%x+) tr=%x+ bl=(%x+) br=%x+"
    )
  do
    idx = tonumber(idx)
    drawn[#drawn + 1] = {
      char = GLYPHS:sub(idx + 1, idx + 1),
      x = tonumber(x),
      y = tonumber(y),
      top = top,
      bottom = bottom,
    }
  end
  return drawn
end

-- The readouts a scene drew. Each is drawn in one go, so a step up or down the
-- screen ends a readout even where two of them share a line.
local function rows(description)
  local out = {}
  local row = nil
  for _, sprite in ipairs(sprites(description)) do
    if row == nil or math.abs(sprite.y - row.y) > ROW_TOLERANCE then
      row = { y = sprite.y }
      out[#out + 1] = row
    end
    row[#row + 1] = sprite
  end
  return out
end

local function spell(description)
  local text = {}
  for _, row in ipairs(rows(description)) do
    local chars = {}
    for _, sprite in ipairs(row) do
      chars[#chars + 1] = sprite.char
    end
    text[#text + 1] = table.concat(chars)
  end
  return table.concat(text, "\n")
end

-- Asserts the color of every character of a readout the caller does not
-- exclude.
local function assert_palette(row, palette, except)
  for _, sprite in ipairs(row) do
    if not (except or ""):find(sprite.char, 1, true) then
      assert(
        sprite.top == palette.top and sprite.bottom == palette.bottom,
        ("%q drew %s..%s"):format(sprite.char, sprite.top, sprite.bottom)
      )
    end
  end
end

-- A readout that reserved room it never drew into is off to one side, so what
-- a row spells is checked against where it sits.
local function assert_centered(row)
  local middle = (row[1].x + row[#row].x) / 2
  local center = trx.ui.canvas.width / 2
  assert(
    math.abs(middle - center) <= ROW_TOLERANCE,
    ("the readout sits at %d, not %d"):format(middle, center)
  )
end

local function color_of(row, char)
  for _, sprite in ipairs(row) do
    if sprite.char == char then
      return sprite
    end
  end
  return nil
end

local function assert_spells(description, expected)
  local got = spell(description)
  assert(
    got == expected,
    ("expected:\n%s\ngot:\n%s\nfrom:\n%s"):format(expected, got, description)
  )
end

test("the run timer spells the clock the level keeps", function()
  local description = draw_after(function()
    fake.set_run(COURSE, 95, 0, 0, 0)
  end)
  assert_spells(description, "0:03.1")
  assert_palette(rows(description)[1], NEUTRAL)
end)

test("a penalty draws in pink, with the mark in the plain color", function()
  local description = draw_after(function()
    fake.set_run(COURSE, 95, 1800, 900, 90)
  end)
  assert_spells(description, "1:00s\nT0:30s\n0:03.1")

  -- The mark the target penalty carries is the one character the engine draws
  -- in the timer's own color rather than the penalty's.
  local target = rows(description)[2]
  assert_palette(target, PINK, "T")
  local mark = color_of(target, "T")
  assert(mark ~= nil, "the target penalty drew no mark")
  assert(mark.top == NEUTRAL.top, mark.top)
  assert(mark.bottom == NEUTRAL.bottom, mark.bottom)
end)

test("the readouts stand down while the level is held still", function()
  local description = draw_after(function()
    fake.set_run(COURSE, 95, 1800, 900, 90)
    fake.set_playing(false)
  end)
  assert_spells(description, "")
end)

test("the penalties line their digits up under one another", function()
  local description = draw_after(function()
    fake.set_run(COURSE, 95, 1800, 900, 90)
  end)

  -- The target penalty carries a mark the other one has not, so the two read
  -- as a column only where the engine's own right edge is kept.
  local penalty, target = rows(description)[1], rows(description)[2]
  local function right_edge(row)
    local edge = row[1].x
    for _, sprite in ipairs(row) do
      edge = math.max(edge, sprite.x)
    end
    return edge
  end
  assert(
    right_edge(penalty) == right_edge(target),
    ("%d ~= %d"):format(right_edge(penalty), right_edge(target))
  )
end)

test(
  "the clock stands in the middle whether a penalty is up or not",
  function()
    local plain = draw_after(function()
      fake.set_run(COURSE, 95, 0, 0, 0)
    end)
    assert_centered(rows(plain)[1])

    -- The penalties take the left, so the clock keeps the middle only while as
    -- much room again is held open on the right.
    local penalized = draw_after(function()
      fake.set_run(COURSE, 95, 1800, 900, 90)
    end)
    assert_centered(rows(penalized)[3])
  end
)

test("a lap with no record on file draws the lap alone", function()
  local description = draw_after(function()
    fake.set_run(QUAD, 0, 0, 0, 0)
    fake.set_lap(450, 60)
  end)
  assert_spells(description, "0:15.0")
  assert_palette(rows(description)[1], NEUTRAL)
  assert_centered(rows(description)[1])
end)

test("a lap draws the record beside it once one is on file", function()
  local description = draw_after(function()
    trx.assault.stats.add_record(20, QUAD)
    fake.set_run(QUAD, 0, 0, 0, 0)
    fake.set_lap(450, 60)
  end)
  assert_spells(description, "0:15.00:20.0")

  -- A lap short of the record is red against it, and the record stays grey.
  local lap = rows(description)[1]
  assert(color_of(lap, "1").top == RED.top)
  assert(color_of(lap, "2").top == GREY.top)
end)

test("a lap that matches the record draws both in green", function()
  local description = draw_after(function()
    trx.assault.stats.add_record(15, QUAD)
    fake.set_run(QUAD, 0, 0, 0, 0)
    fake.set_lap(450, 60)
  end)
  assert_spells(description, "0:15.00:15.0")
  assert_palette(rows(description)[1], GREEN)
end)

test("the run timer stands down while the lap times are up", function()
  local description = draw_after(function()
    fake.set_run(QUAD, 95, 0, 0, 0)
    fake.set_lap(0, 60)
  end)
  assert_spells(description, "")
end)

return h.report()
