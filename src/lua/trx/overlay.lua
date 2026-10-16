local raw = trxc.overlay
local h = require("trx.internal.helpers")
local lasting_polled = require("trx.signal").lasting_polled

---@class (partial) trx
---@field overlay trx.overlay

---What the game shows over the play: the pickups that slide in, the cinematic
---bars, and the lines of text and the arrows that the inventory ring, a demo
---or a film asks for. The shipped scripts draw all of it; the signals here
---say what to draw.
---@trx.module 41 Overlay
---@class (exact) trx.overlay
---@trx.readonly has_letterbox
---@field has_letterbox boolean Whether the cinematic bars take any of the screen. It stays true while they move, so a script can hold something back until they have gone.
---@field letterbox number How deep the cinematic bars are asked to be, as a fraction of the screen height each, from `0` to `0.5`. Setting it slides the bars there; `0` takes them away. Flyby cameras and cutscenes set it too, and the last request wins. `trx.ui.working_area` follows the bars as they move.
local M = h.module("overlay")

---What the overlay tells a script, for the parts of it a script draws.
---@class (exact) trx.overlay.signals
---@trx.readonly arrows, caption, caption_flashing, health_bar_forced, letterbox, top_text, version_shown
---@field top_text trx.signal.Signal Holds the line the game shows at the top of the screen, such as the name of the inventory ring, in the player's language, or nil for none.
---@field caption trx.signal.Signal Holds the caption at the bottom of the screen, such as the name of the entry under the cursor of the inventory ring, in the player's language, or nil for none.
---@field caption_flashing trx.signal.Signal Holds whether the caption flashes, as it does while a demo plays.
---@field arrows trx.signal.Signal Holds the arrows on screen, as a table that maps each `trx.overlay.Arrow` that shows to `true`.
---@field version_shown trx.signal.Signal Holds whether the game asks for its version on screen, as the title screen does. The `ui.show_title_version` setting decides whether it shows. <!--noref: ui.show_title_version-->
---@field letterbox trx.signal.Signal Says when the cinematic bars take the screen and when they give it back. True while they are moving, so a script can hold something back until they have gone.
---@field health_bar_forced trx.signal.Signal Says when something asks for Lara's health bar whatever else is on screen, which the inventory ring does while it shows a medipack.
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

-- The arrows on screen, as a number with one bit for each, so that the poll
-- says only when one of them moves.
local function arrow_bits()
  local bits = 0
  for _, arrow in pairs(M.Arrow) do
    if raw.is_arrow_shown(arrow) then
      bits = bits | (1 << arrow)
    end
  end
  return bits
end

local polled_arrow_bits = lasting("arrow_bits", function()
  return lasting_polled(arrow_bits)
end)

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
function M.set_caption(text)
  raw.set_caption(text)
  -- The signal takes the caption at once rather than on the next tick, so
  -- that it shows on the same frame as whatever asked for it.
  M.signals.caption:set(raw.caption())
end

---Shows or hides an interface arrow.
---
---The caption arrows stand beside the caption that `trx.overlay.set_caption`
---sets.
---@param arrow trx.overlay.Arrow Which arrow to show.
---@param shown boolean Whether it is on screen.
function M.show_arrow(arrow, shown)
  raw.show_arrow(arrow, shown)
  -- The signal takes the arrows at once, as `trx.overlay.set_caption` does.
  polled_arrow_bits():set(arrow_bits())
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
  top_text = {
    get = lasting("top_text", function()
      return lasting_polled(raw.top_text)
    end),
  },
  caption = {
    get = lasting("caption", function()
      return lasting_polled(raw.caption)
    end),
  },
  caption_flashing = {
    get = lasting("caption_flashing", function()
      return lasting_polled(raw.is_caption_flashing)
    end),
  },
  arrows = {
    get = lasting("arrows", function()
      return polled_arrow_bits():map(function(bits)
        local shown = {}
        for _, arrow in pairs(M.Arrow) do
          if bits & (1 << arrow) ~= 0 then
            shown[arrow] = true
          end
        end
        return shown
      end)
    end),
  },
  version_shown = {
    get = lasting("version_shown", function()
      return lasting_polled(raw.is_version_shown)
    end),
  },
  letterbox = {
    get = lasting("letterbox", function()
      return lasting_polled(raw.has_letterbox)
    end),
  },
  health_bar_forced = {
    get = lasting("health_bar_forced", function()
      return lasting_polled(raw.is_health_bar_forced)
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
