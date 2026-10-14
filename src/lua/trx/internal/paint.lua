-- Paint passes that engine scripts reach after the API is sealed.
--
-- Captured at module scope: the raw C bridge is removed from the globals once
-- the trx.* modules have loaded, and the engine's own UI scripts run after
-- that.
local raw_events = trxc.events
local h = require("trx.internal.helpers")

local console_event = h.constants("LUA_EVENT_TYPE").UI_PAINT_CONSOLE

local M = {}

-- Attaches a painter to the console's layer, which draws over every other
-- layer of the interface but the game-exit fade.
function M.on_paint_console(callback)
  return raw_events.attach(console_event, callback)
end

return M
