local raw = trxc.events
local types = trxc.events.EventType

local Event = {}
function Event.__call(self, callback)
  local et = self._type
  return raw.attach(et, callback)
end

local events = { EventType = types }
for name, et in pairs(types) do
  local proxy = { _type = et }
  setmetatable(proxy, Event)
  events["on_" .. string.lower(name)] = proxy
end

function events.detach(id)
  raw.detach(id)
end

trx.events = events
