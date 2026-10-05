require("trx.signal")

local raw = trxc.overlay
local h = require("trx.internal.helpers")

---@class trx
---@field overlay trx.overlay

---What the engine draws over the game and no script owns: the pickups that
---slide in, the assault course digits, and the lines of text the rest of the
---engine asks for.
---
---Only what a script has to answer for is here. The lines of text are the
---engine's own, and a script neither reads nor writes them.
---@trx.module 41 Overlay
---@class (exact) trx.overlay
---@trx.readonly has_letterbox
---@field has_letterbox boolean Whether the cinematic bars take any of the screen. It stays true while they move, so a script can hold something back until they have gone.
local M = h.module("overlay")

---What the overlay tells a script, for the parts of it a script draws.
---@class (exact) trx.overlay.signals
---@trx.readonly health_bar_forced, letterbox
---@field letterbox trx.signal.Signal Says when the cinematic bars take the screen and when they give it back. True while they are moving, so a script can hold something back until they have gone.
---@field health_bar_forced trx.signal.Signal Says when something asks for Lara's health bar whatever else is on screen, which the inventory ring does while it shows a medipack.
M.signals = h.namespace("overlay.signals")

local held = nil
local letterbox_held = nil

---An arrow the interface can show.
---@enum trx.overlay.Arrow
local Arrow = {
  ---The top-left screen corner.
  TOP_LEFT = h.IntegerConstant,
  ---The top-right screen corner.
  TOP_RIGHT = h.IntegerConstant,
  ---The bottom-left screen corner.
  BOTTOM_LEFT = h.IntegerConstant,
  ---The bottom-right screen corner.
  BOTTOM_RIGHT = h.IntegerConstant,
  ---To the left of the caption.
  CAPTION_LEFT = h.IntegerConstant,
  ---To the right of the caption.
  CAPTION_RIGHT = h.IntegerConstant,
}
M.Arrow = h.enum("overlay.Arrow", "OVERLAY_ARROW", Arrow)

---Sets the caption at the bottom of the screen.
---
---The inventory ring uses it for the selected entry. The caption reduces the
---safe area. Passing no value removes it.
---@param text? string Text to show. Omit this parameter to remove the caption.
---@type fun(text?: string)
M.set_caption = raw.set_caption

---Shows or hides an interface arrow.
---
---The caption arrows stand beside the caption that `trx.overlay.set_caption`
---sets.
---@param arrow trx.overlay.Arrow Which arrow to show.
---@param shown boolean Whether it is on screen.
---@type fun(arrow: trx.overlay.Arrow, shown: boolean)
M.show_arrow = raw.show_arrow

h.properties(M, "overlay", {
  has_letterbox = {
    get = raw.has_letterbox,
  },
})

h.properties(M.signals, "overlay.signals", {
  letterbox = {
    get = function()
      if letterbox_held == nil then
        letterbox_held = trx.signal.polled(raw.has_letterbox)
      end
      return letterbox_held
    end,
  },
  health_bar_forced = {
    get = function()
      if held == nil then
        held = trx.signal.polled(raw.is_health_bar_forced)
      end
      return held
    end,
  },
})

---Shows an object in the bottom-right corner of the screen, as the game shows
---a pickup Lara collects. It fires `trx.events.on_show_pickup`. It also plays
---secret music for a secret pickup.
---
---It does not add the object to Lara's inventory. Use
---`trx.inventory.Inventory:give` for that.
---
---```lua
---trx.overlay.show_pickup(trx.catalog.objects.KEY_ITEM_1)
---```
---@param object trx.catalog.objects The object to show.
---@type fun(object: trx.catalog.objects)
M.show_pickup = raw.show_pickup
