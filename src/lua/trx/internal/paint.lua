-- Paint passes that engine scripts reach after the API is sealed.
--
-- Captured at module scope: the raw C bridge is removed from the globals once
-- the trx.* modules have loaded, and the engine's own UI scripts run after
-- that.
local raw_events = trxc.events

local console_event = 0
for _, constant in ipairs(trxc.enum.values("LUA_EVENT_TYPE")) do
  if constant.name == "UI_PAINT_CONSOLE" then
    console_event = constant.value
  end
end

local M = {}

-- Attaches a painter to the console's layer, which draws over every other
-- layer of the interface but the game-exit fade.
function M.on_paint_console(callback)
  return raw_events.attach(console_event, callback)
end

return M
