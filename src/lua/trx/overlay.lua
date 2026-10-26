local raw = trxc.overlay
local h = require("trx.internal.helpers")
local lasting_polled = require("trx.signal").lasting_polled

---@class (partial) trx
---@field overlay trx.overlay

---What the game shows over the play: the pickups that slide in, the cinematic
---bars, and the caption and the arrows that the inventory ring, a demo or an
---FMV asks for. The shipped scripts draw all of it; the signals here
---say what to draw.
---@trx.module 41 Overlay
---@class (exact) trx.overlay
---@trx.readonly has_letterbox
---@field has_letterbox boolean Whether the cinematic bars take any of the screen. It stays true while they move, so a script can hold something back until they have gone.
---@field letterbox number How deep the cinematic bars are asked to be, as a fraction of the screen height each, from `0` to `0.5`. Setting it slides the bars there; `0` takes them away. Flyby cameras and cutscenes set it too, and the last request wins. `trx.ui.working_area` follows the bars as they move.
local M = h.module("overlay")

---What the overlay tells a script, for the parts of it a script draws.
---@class (exact) trx.overlay.signals
---@trx.readonly arrows, caption, caption_flashing, letterbox, version_shown
---@field caption trx.signal.Signal Holds the caption at the bottom of the screen, such as the name of the entry under the cursor of the inventory ring, in the player's language, or nil for none.
---@field caption_flashing trx.signal.Signal Holds whether the caption flashes, as it does while a demo plays.
---@field arrows trx.signal.Signal Holds the arrows on screen, as a table that maps each `trx.overlay.Arrow` that shows to `true`.
---@field version_shown trx.signal.Signal Holds whether the game asks for its version on screen, as the title screen does. The `ui.show_title_version` setting decides whether it shows. <!--noref: ui.show_title_version-->
---@field letterbox trx.signal.Signal Says when the cinematic bars take the screen and when they give it back. True while they are moving, so a script can hold something back until they have gone.
M.signals = h.namespace("overlay.signals")

local polled = {}

-- One signal for each reader, made when a script first asks for it.
local function lasting(name, read)
  return function()
    if polled[name] == nil then
      polled[name] = read()
    end
    return polled[name]
  end
end

---An arrow the interface can show.
---@enum trx.overlay.Arrow
local Arrow = {
  ---The top-left screen corner.
  TOP_LEFT = 0,
  ---The top-right screen corner.
  TOP_RIGHT = 1,
  ---The bottom-left screen corner.
  BOTTOM_LEFT = 2,
  ---The bottom-right screen corner.
  BOTTOM_RIGHT = 3,
  ---To the left of the caption.
  CAPTION_LEFT = 4,
  ---To the right of the caption.
  CAPTION_RIGHT = 5,
}
M.Arrow = h.coined_enum("overlay.Arrow", Arrow)

local is_arrow = {}
for _, arrow in pairs(M.Arrow) do
  is_arrow[arrow] = true
end

local caption = trx.signal.new(nil)
local caption_flashing = trx.signal.new(false)

---@class (exact) trx.overlay.set_caption.opts
---@field flashing? boolean Whether the caption flashes, as it does while a demo plays. `false` by default.

---Sets the caption at the bottom of the screen.
---
---The inventory ring uses it for the selected entry. The text shows as given,
---so the caller styles it. The caption reduces the safe area. Passing no value
---removes it.
---
---```lua
---trx.overlay.set_caption(trx.locale.get("general/misc/demo_mode"), {
---  flashing = true,
---})
---```
---@param text? string|trx.locale.Text Text to show. A `trx.locale.Text` follows the player's language. Omit this parameter to remove the caption.
---@param opts? trx.overlay.set_caption.opts How it shows.
function M.set_caption(text, opts)
  opts = opts or {}
  caption:set(text)
  caption_flashing:set(text ~= nil and opts.flashing == true)
end

local arrows = trx.signal.new({})
local version_shown = trx.signal.new(false)

---Asks for the game's version on screen, as the title screen does. The
---`ui.show_title_version` setting decides whether it shows. <!--noref: ui.show_title_version-->
---@param shown boolean Whether it is asked for.
function M.show_version(shown)
  version_shown:set(shown == true)
end

-- A caption or an arrow that its screen left up goes with the level.
trx.events.on_level_unload(function()
  M.set_caption()
  arrows:set({})
end)

---Shows or hides an interface arrow.
---
---The caption arrows stand beside the caption that `trx.overlay.set_caption`
---sets.
---@param arrow trx.overlay.Arrow Which arrow to show.
---@param shown boolean Whether it is on screen.
function M.show_arrow(arrow, shown)
  if not is_arrow[arrow] then
    error("unknown arrow", 2)
  end
  shown = shown == true
  local now = arrows:get()
  if (now[arrow] == true) == shown then
    return
  end
  local next_arrows = {}
  for other in pairs(now) do
    next_arrows[other] = true
  end
  next_arrows[arrow] = shown or nil
  arrows:set(next_arrows)
end

h.properties(M, "overlay", {
  has_letterbox = {
    get = raw.has_letterbox,
  },
  letterbox = {
    get = raw.get_letterbox,
    set = raw.set_letterbox,
  },
})

h.properties(M.signals, "overlay.signals", {
  caption = {
    get = function()
      return caption
    end,
  },
  caption_flashing = {
    get = function()
      return caption_flashing
    end,
  },
  arrows = {
    get = function()
      return arrows
    end,
  },
  version_shown = {
    get = function()
      return version_shown
    end,
  },
  letterbox = {
    get = lasting("letterbox", function()
      return lasting_polled(raw.has_letterbox)
    end),
  },
})

---Shows an object in the bottom-right corner of the screen, as the game shows
---a pickup Lara collects. It fires `trx.events.on_show_pickup`, which the
---shipped scripts answer by playing the secret music for a secret.
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
