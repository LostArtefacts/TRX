local raw = trxc.items

-- Item proxy metatable
local getters = {
  pos = raw.get_pos,
  rot = raw.get_rot,
  room = raw.get_room,
  status = raw.get_status,
  object_id = raw.get_object_id,
  hit_points = raw.get_hit_points,
  max_hit_points = raw.get_max_hit_points,
  name = raw.get_name,
}

local setters = {
  pos = raw.set_pos,
  rot = raw.set_rot,
  hit_points = raw.set_hit_points,
  max_hit_points = raw.set_max_hit_points,
  name = raw.set_name,
}

local Item = {}

Item.__index = function(self, key)
  local getter = getters[key]
  return getter and getter(self.idx) or nil
end

Item.__newindex = function(self, key, value)
  local setter = setters[key]
  if setter then
    setter(self.idx, value)
    return
  end
  error("Cannot set field '" .. key .. "' on trx.items.Item")
end

-- items metatable - functions
local fn = {}

function fn.get(arg)
  local idx = raw.get(arg)
  if not idx then
    return nil
  end
  return setmetatable({ idx = idx }, Item)
end

-- items metatable - metamethods
trx.items = setmetatable({}, {
  Item = Item,
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
