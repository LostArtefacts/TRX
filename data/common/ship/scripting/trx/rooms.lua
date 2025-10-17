local raw = trxc.rooms

-- Room proxy metatable
local getters = {
  flags = raw.get_flags,
}

local Room = {}

Room.__index = function(self, key)
  local getter = getters[key]
  return getter and getter(self.idx) or nil
end

-- rooms metatable - functions
local fn = {}

function fn.get(arg)
  local idx = raw.get(arg)
  if not idx then
    return nil
  end
  return setmetatable({ idx = idx }, Room)
end

trx.rooms = setmetatable({}, {
  __len = function()
    return raw.count()
  end,
  __index = function(_, key)
    if key == "fn" then
      return fn
    elseif type(key) == "number" or type(key) == "string" then
      return fn.get(key)
    end
    return nil
  end,
})
