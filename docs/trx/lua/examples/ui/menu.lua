-- Asks the player to pick a difficulty as the level starts, and logs the
-- choice. Backing out keeps the normal difficulty.

local CHOICES = { "Easy", "Normal", "Hard" }

local rows = {}
for i, choice in ipairs(CHOICES) do
  rows[i] = { text = choice }
end

local list = trx.ui.widgets.List({ rows = rows, min_w = 120 })
list:select(2)

local menu = trx.ui.widgets.Frame({
  child = trx.ui.widgets.Pad({
    x = 4,
    y = 4,
    child = trx.ui.widgets.Stack({
      align = trx.ui.HAlign.CENTER,
      spacing = 4,
      children = {
        trx.ui.widgets.Label({ text = "Difficulty" }),
        list,
      },
    }),
  }),
})

trx.ui.layers.push({
  root = menu,
  on_input = function(layer, keys)
    local picked, cancelled = list:control(keys)
    if cancelled then
      picked = 2
    end
    if picked ~= nil then
      trx.log.info("Difficulty: " .. CHOICES[picked])
      layer:close()
    end
  end,
})
