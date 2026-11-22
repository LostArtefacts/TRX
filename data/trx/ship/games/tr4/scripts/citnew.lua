local cutscenes = require("tr4.cutscenes")

local JEAN_YVES_CUTSCENE = 16

cutscenes.register(JEAN_YVES_CUTSCENE, {
  on_start = function()
    trx.cutscenes.set_lara_return({ x = 61868, y = 0, z = 44994 }, -16060)
  end,
})
