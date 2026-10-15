-- The shipped text dialog, which shows a title over a text a page at a time.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
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

test("a long text runs to several pages", function()
  local dialog = text_dialog.new()
  dialog:set_text("T", LONG, 1000, 2)
  assert(#dialog.pages:get() == 3, #dialog.pages:get())
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

return h.report()
