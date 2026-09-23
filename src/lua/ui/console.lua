require("trx.console")
require("trx.events")
require("trx.game")
require("trx.math")
require("trx.input")
require("trx.ui")
require("trx.ui.primitive")

local primitive = trx.ui.primitive

local PADDING = 5.0
local TEXT_HEIGHT = 15
local LOG_SCALE = 0.8
local MAX_LOG_LINES = 20
local SECONDS_PER_CHAR = 0.2
local MIN_SECONDS = 2.0
local MAX_SECONDS = 15.0
local SPACING = 8.0
local BACKDROP_ALPHA = 196
local CARET = "\\{button left}"
local BLINK_TICKS = 10

-- Returns the text scale used by the engine.
local function text_scale()
  local _, height = primitive.measure_text("", 1.0)
  return height / TEXT_HEIGHT
end

-- Returns the available width for wrapped log text.
local function room_to_wrap(scale)
  return trx.ui.canvas.width - 2 * PADDING * scale
end

-------------------------------------------------------------------------------
-- Prompt
-------------------------------------------------------------------------------
local line = (function()
  local self = { text = "", caret = 1 }

  local completion = nil
  local history_idx = nil

  function self.clear()
    self.text = ""
    self.caret = 1
    completion = nil
  end

  function self.take(text)
    self.text = text
    self.caret = #text + 1
    completion = nil
  end

  function self.insert(text)
    self.text = self.text:sub(1, self.caret - 1)
      .. text
      .. self.text:sub(self.caret)
    self.caret = self.caret + #text
    completion = nil
  end

  function self.prev_caret()
    return utf8.offset(self.text, -1, self.caret) or 1
  end

  function self.next_caret()
    return utf8.offset(self.text, 2, self.caret) or #self.text + 1
  end

  function self.move_to(caret)
    self.caret = caret
    completion = nil
  end

  function self.delete_back()
    if self.caret <= 1 then
      return
    end
    local from = self.prev_caret()
    self.text = self.text:sub(1, from - 1) .. self.text:sub(self.caret)
    self.caret = from
    completion = nil
  end

  function self.before_caret()
    return self.text:sub(1, self.caret - 1)
  end

  local function apply_completion(replacement)
    self.text = self.text:sub(1, completion.start - 1)
      .. replacement
      .. self.text:sub(completion.stop)
    self.caret = completion.start + #replacement
    completion.stop = completion.start + #replacement
  end

  function self.complete(step)
    if completion == nil then
      local suggestions, start, stop =
        trx.console.complete(self.text, self.caret - 1)
      if suggestions == nil or #suggestions == 0 then
        return
      end
      completion = {
        suggestions = suggestions,
        start = start + 1,
        stop = stop + 1,
        index = step > 0 and 1 or #suggestions,
      }
      completion.original =
        self.text:sub(completion.start, completion.stop - 1)
    else
      local count = #completion.suggestions + 1
      completion.index = ((completion.index - 1 + step + count) % count) + 1
    end
    apply_completion(
      completion.suggestions[completion.index] or completion.original
    )
  end

  function self.recall(step)
    local entered = trx.console.history()
    local at = (history_idx or #entered + 1) + step
    if at < 1 then
      at = 1
    elseif at > #entered + 1 then
      at = #entered + 1
    end
    history_idx = at
    self.take(entered[at] or "")
  end

  function self.forget_recall()
    history_idx = nil
  end

  return self
end)()

-------------------------------------------------------------------------------
-- Logs
-------------------------------------------------------------------------------
local logs = (function()
  local self = { entries = {} }

  local width = nil
  local scale = nil

  local function wrap_paragraph(text, room, out)
    local line_start, line_stop = nil, nil
    local at = 1
    while true do
      local word_start, word_stop = text:find("%S+", at)
      if word_start == nil then
        break
      end
      if line_start == nil then
        line_start, line_stop = word_start, word_stop
      elseif
        primitive.measure_text(text:sub(line_start, word_stop), LOG_SCALE)
        > room
      then
        out[#out + 1] = text:sub(line_start, line_stop)
        line_start, line_stop = word_start, word_stop
      else
        line_stop = word_stop
      end
      at = word_stop + 1
    end
    if line_start ~= nil then
      out[#out + 1] = text:sub(line_start, line_stop)
    end
  end

  local function wrap(text, room)
    if room <= 0 then
      return { text }
    end
    local out = {}
    local at = 1
    while at <= #text + 1 do
      local brk = text:find("\n", at, true)
      local paragraph = text:sub(at, (brk or #text + 1) - 1)
      local before = #out
      wrap_paragraph(paragraph, room, out)
      if #out == before then
        out[#out + 1] = ""
      end
      if brk == nil then
        break
      end
      at = brk + 1
    end
    if #out == 0 then
      out[1] = text
    end
    return out
  end

  -- Returns the expiration time for a log entry.
  local function expiry(text)
    local seconds = (utf8.len(text) or #text) * SECONDS_PER_CHAR
    seconds = math.max(MIN_SECONDS, math.min(MAX_SECONDS, seconds))
    return trx.game.real_time + seconds
  end

  function self.reflow(new_width, new_scale)
    if new_width == width and new_scale == scale then
      return
    end
    width, scale = new_width, new_scale
    for _, entry in ipairs(self.entries) do
      entry.lines = wrap(entry.text, width)
    end
  end

  function self.add(text)
    local at_scale = text_scale()
    self.reflow(room_to_wrap(at_scale), at_scale)
    table.insert(self.entries, 1, {
      text = text,
      lines = wrap(text, width),
      expires_at = expiry(text),
    })
    while #self.entries > MAX_LOG_LINES do
      table.remove(self.entries)
    end
  end

  function self.clear()
    self.entries = {}
  end

  function self.drop_expired()
    local now = trx.game.real_time
    local kept = {}
    for _, entry in ipairs(self.entries) do
      if entry.expires_at > now then
        kept[#kept + 1] = entry
      end
    end
    self.entries = kept
  end

  return self
end)()

-------------------------------------------------------------------------------
-- What the player sees
-------------------------------------------------------------------------------
local grab = nil
local ticks = 0

local function backdrop(line_height)
  local canvas = trx.ui.canvas
  local height = line_height + 7 * line_height * LOG_SCALE
  -- The canvas holds a whole number of units, so its right and bottom edges
  -- stop short of the screen by the fraction that was dropped. Reaching one
  -- unit past both always covers it.
  local bleed = 1
  local top = { r = 0, g = 0, b = 0, a = 0 }
  local bottom = { r = 0, g = 0, b = 0, a = BACKDROP_ALPHA }
  primitive.gradient_quad(
    0,
    canvas.height - height,
    0,
    canvas.width + bleed,
    height + bleed,
    top,
    top,
    bottom,
    bottom
  )
end

local function draw()
  logs.drop_expired()
  local open = trx.console.is_open
  if not open and #logs.entries == 0 then
    return
  end

  local canvas = trx.ui.canvas
  local scale = text_scale()
  local padding = PADDING * scale
  local line_height = TEXT_HEIGHT * scale
  logs.reflow(room_to_wrap(scale), scale)
  backdrop(line_height)

  local baseline = canvas.height - padding - line_height
  if open then
    primitive.text(line.text, padding, baseline, 1.0, 16)
    if (ticks % (BLINK_TICKS * 2)) < BLINK_TICKS then
      local at = primitive.measure_text(line.before_caret(), 1.0)
      primitive.text(CARET, padding + at, baseline, 1.0, 8)
    end
  end

  -- Draw entries from oldest to newest, upward from the prompt.
  local y = baseline - SPACING * scale
  for i = 1, #logs.entries do
    local lines = logs.entries[i].lines
    for j = #lines, 1, -1 do
      y = y - line_height * LOG_SCALE
      primitive.text(lines[j], padding, y, LOG_SCALE, 16)
    end
  end
end

-------------------------------------------------------------------------------
-- What the player does
-------------------------------------------------------------------------------
local function confirm()
  local entered = line.text
  line.clear()
  if entered ~= "" then
    trx.console.remember(entered)
    pcall(trx.console.eval, entered, { verbose = true })
  end
  trx.console.is_open = false
end

local KEYS = {
  ["left"] = function()
    line.move_to(line.prev_caret())
  end,
  ["right"] = function()
    line.move_to(line.next_caret())
  end,
  ["home"] = function()
    line.move_to(1)
  end,
  ["end"] = function()
    line.move_to(#line.text + 1)
  end,
  ["backspace"] = function()
    line.delete_back()
  end,
  ["up"] = function()
    line.recall(-1)
  end,
  ["down"] = function()
    line.recall(1)
  end,
  ["return"] = confirm,
  ["keypad enter"] = confirm,
  ["escape"] = function()
    line.clear()
    trx.console.is_open = false
  end,
  ["v"] = function()
    if
      trx.input.is_key_held("left ctrl") or trx.input.is_key_held("right ctrl")
    then
      line.insert(trx.ui.clipboard)
    end
  end,
  ["tab"] = function()
    local shift = trx.input.is_key_held("left shift")
      or trx.input.is_key_held("right shift")
    line.complete(shift and -1 or 1)
  end,
}

local function handle_key(key)
  if not trx.console.is_open then
    return
  end
  local handler = KEYS[key]
  if handler ~= nil then
    handler()
  end
end

trx.events.on_console_open(function()
  line.clear()
  line.forget_recall()
  ticks = 0
  if grab == nil then
    grab = trx.input.grab()
  end
end)

trx.events.on_console_close(function()
  line.clear()
  if grab ~= nil then
    grab:release()
    grab = nil
  end
end)

trx.events.on_console_log(function(text)
  logs.add(text)
end)

trx.events.on_console_clear(function()
  logs.clear()
end)

trx.events.on_key_down(handle_key)
trx.events.on_key_repeat(handle_key)

trx.events.on_text_input(function(text)
  if trx.console.is_open then
    line.insert(text)
  end
end)

trx.events.on_tick(function()
  ticks = ticks + 1
end)

trx.events.on_level_unload(function()
  grab = nil
  if trx.console.is_open then
    grab = trx.input.grab()
  end
end)

trx.events.on_ui_paint_over(draw)
