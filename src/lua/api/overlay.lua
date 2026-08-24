require("trx.signal")

local raw = trxc.overlay
local api = trx.api

api.module("overlay", {
  order = 39,
  title = "Overlay",
  description = "What the engine draws over the game and no script owns: the pickups that "
    .. "slide in, the assault course digits, and the lines of text the rest of the engine "
    .. "asks for.\n\nOnly what a script has to answer for is here. The lines of text are the "
    .. "engine's own, and a script neither reads nor writes them.",
})

api.namespace("overlay.signals", {
  description = "What the overlay tells a script, for the parts of it a script draws.",
})

local held = nil

api.property("overlay.signals.health_bar_forced", {
  type = "signal.Signal",
  description = "Says when something asks for Lara's health bar whatever else is on screen, "
    .. "which the inventory ring does while it shows a medipack.",
  get = function()
    if held == nil then
      held = trx.signal.polled(raw.is_health_bar_forced)
    end
    return held
  end,
})
