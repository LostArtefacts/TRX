-- The shipped text dialog, which shows a title over a text a page at a time.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local accent = require("common.ui.accent")
local text_dialog = require("common.ui.widgets.text_dialog")

local LONG = "one\ntwo\nthree\nfour\nfive"

local function press(role)
  fake.release_all()
  fake.press(role)
  fake.tick()
  fake.release_all()
  fake.tick()
end

test("a dialog shows its title in capitals", function()
  local dialog = text_dialog.new()
  dialog:set_text("Title", "text", 1000, 4)
  assert(dialog.title:get() == "TITLE")
  assert(#dialog.pages:get() == 1)
end)

test("a dialog without a title draws none", function()
  local function draws_title(settings)
    local dialog = text_dialog.new(settings)
    dialog:set_text("Title", "text", 1000, 4)
    local layer = trx.ui.layers.push({ root = dialog.widget })
    fake.tick()
    local found = false
    for _, op in ipairs(fake.scene()) do
      found = found or op:find("text=TITLE", 1, true) ~= nil
    end
    layer:close()
    return found
  end
  assert(draws_title({}), "a titled dialog drew no title")
  assert(not draws_title({ titled = false }), "the title showed")
end)

test("a long text runs to several pages", function()
  local dialog = text_dialog.new()
  dialog:set_text("T", LONG, 1000, 2)
  assert(#dialog.pages:get() == 3, #dialog.pages:get())
end)

test("each page keeps the colour the text sets", function()
  local dialog = text_dialog.new()
  dialog:set_text("T", "\\{color #c08040}" .. LONG, 1000, 2)
  local pages = dialog.pages:get()
  for i, page in ipairs(pages) do
    assert(
      page:find("\\{color #c08040}", 1, true) == 1,
      "page " .. i .. ": " .. page
    )
  end
end)

test("left and right turn the pages and stop at the ends", function()
  local dialog = text_dialog.new({ heavy = true })
  dialog:set_text("T", LONG, 1000, 2)
  local layer = trx.ui.layers.push({
    root = dialog.widget,
    on_input = function(_, keys)
      dialog:control(keys)
    end,
  })
  fake.tick()
  press(Role.MENU_LEFT)
  assert(dialog.page:get() == 1)
  press(Role.MENU_RIGHT)
  press(Role.MENU_RIGHT)
  press(Role.MENU_RIGHT)
  assert(dialog.page:get() == 3)
  layer:close()
end)

test("the same text wrapped again keeps the page", function()
  local dialog = text_dialog.new()
  dialog:set_text("T", LONG, 1000, 2)
  dialog.page:set(2)
  dialog:set_text("T", LONG, 900, 2)
  assert(dialog.page:get() == 2)
  dialog:set_text("T", "other", 900, 2)
  assert(dialog.page:get() == 1)
end)

test("a dialog shows its title in orange in TR4 only", function()
  for _, case in ipairs({ { version = 4, orange = true }, { version = 1 } }) do
    fake.set_tr_version(case.version)
    local dialog = text_dialog.new()
    dialog:set_text("Title", "text", 1000, 4)
    local layer = trx.ui.layers.push({ root = dialog.widget })
    fake.tick()
    local title = nil
    for _, op in ipairs(fake.scene()) do
      title = title or op:match("^text .- text=(.*TITLE.*)$")
    end
    layer:close()
    assert(title ~= nil, "no title")
    local color = accent.color_of(accent.Role.VALUE)
    local orange = color ~= nil
      and title:find(accent.color("", color), 1, true) ~= nil
    assert(
      orange == (case.orange == true),
      ("TR%d shows %s"):format(case.version, title)
    )
  end
end)

return h.report()
