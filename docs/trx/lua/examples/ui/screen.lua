-- Draws the pause screen in place of the one the game ships: a heading and
-- two choices. Backing out resumes the game. The shipped pause screen comes
-- back when the level ends.

trx.ui.screens.define(trx.ui.Screen.PAUSE, function(ctx)
  local list = trx.ui.widgets.List({
    rows = { { text = "Resume" }, { text = "Exit to title" } },
    min_w = 140,
  })
  return ctx:push({
    root = trx.ui.widgets.Frame({
      child = trx.ui.widgets.Pad({
        x = 4,
        y = 4,
        child = trx.ui.widgets.Stack({
          align = trx.ui.HAlign.CENTER,
          spacing = 4,
          children = { trx.ui.widgets.Label({ text = "Paused" }), list },
        }),
      }),
    }),
    on_input = function(_, keys)
      local picked, cancelled = list:control(keys)
      if picked == 1 or cancelled then
        ctx:resume()
      elseif picked == 2 then
        ctx:exit_to_title()
      end
    end,
  })
end, { override = true })
