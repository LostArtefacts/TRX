-- Shows the objective of the level in the top right corner, for as long as
-- the level runs.

local objective = trx.ui.widgets.Frame({
  child = trx.ui.widgets.Pad({
    x = 6,
    y = 3,
    child = trx.ui.widgets.Label({ text = "Find the three keys" }),
  }),
})

trx.ui.regions.place(trx.ui.Region.TOP_RIGHT, objective)
