-- Shows a note in the middle of the screen as the level starts. The note
-- closes when the player confirms it or backs out of it.

local Role = trx.input.Role

local note = trx.ui.widgets.Frame({
  child = trx.ui.widgets.Pad({
    x = 8,
    y = 6,
    child = trx.ui.widgets.Stack({
      align = trx.ui.HAlign.CENTER,
      spacing = 6,
      children = {
        trx.ui.widgets.Label({ text = "The gate is locked." }),
        trx.ui.widgets.InputHint({ input = Role.MENU_CONFIRM, text = "Close" }),
      },
    }),
  }),
})

trx.ui.layers.push({
  root = note,
  on_input = function(layer, keys)
    if keys:pressed(Role.MENU_CONFIRM) or keys:pressed(Role.MENU_BACK) then
      layer:close()
    end
  end,
})
