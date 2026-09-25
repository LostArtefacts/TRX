require("trx.signal")

local raw = trxc.overlay
local api = trx.api

api.module("overlay", {
  order = 40,
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
local letterbox_held = nil

api.enum("overlay.Arrow", {
  backing = "OVERLAY_ARROW",
  description = "An arrow the interface can show.",
  values = {
    TOP_LEFT = "The top-left screen corner.",
    TOP_RIGHT = "The top-right screen corner.",
    BOTTOM_LEFT = "The bottom-left screen corner.",
    BOTTOM_RIGHT = "The bottom-right screen corner.",
    CAPTION_LEFT = "To the left of the caption.",
    CAPTION_RIGHT = "To the right of the caption.",
  },
})

api.define("overlay.set_caption", {
  description = [[
Sets the caption at the bottom of the screen.

The inventory ring uses it for the selected entry. The passport uses it for the
current page. The caption reduces the safe area. Passing no value removes it.]],
  params = {
    {
      name = "text",
      type = "string",
      optional = true,
      description = "Text to show. Omit this parameter to remove the caption.",
    },
  },
  impl = raw.set_caption,
})

api.define("overlay.show_arrow", {
  description = [[
Shows or hides an interface arrow.

The passport uses the caption arrows to show which way the book turns.]],
  params = {
    {
      name = "arrow",
      type = "overlay.Arrow",
      description = "Which arrow to show.",
    },
    {
      name = "shown",
      type = "boolean",
      description = "Whether it is on screen.",
    },
  },
  impl = raw.show_arrow,
})

api.property("overlay.has_letterbox", {
  type = "boolean",
  description = [[Whether the cinematic bars take any of the screen. It stays true while they
move, so a script can hold something back until they have gone.]],
  get = raw.has_letterbox,
})

api.property("overlay.signals.letterbox", {
  type = "signal.Signal",
  description = [[Says when the cinematic bars take the screen and when they give it back. True
while they are moving, so a script can hold something back until they have gone.]],
  get = function()
    if letterbox_held == nil then
      letterbox_held = trx.signal.polled(raw.has_letterbox)
    end
    return letterbox_held
  end,
})

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

api.define("overlay.show_pickup", {
  description = [[
Shows an object in the bottom-right corner of the screen, as the game shows a
pickup Lara collects. It fires `trx.events.on_show_pickup`. It also plays secret
music for a secret pickup.

It does not add the object to Lara's inventory. Use `trx.inventory.Inventory:give`
for that.]],
  params = {
    {
      name = "object",
      type = "catalog.objects",
      description = "The object to show.",
    },
  },
  examples = {
    [[trx.overlay.show_pickup(trx.catalog.objects.KEY_ITEM_1)]],
  },
  impl = raw.show_pickup,
})
