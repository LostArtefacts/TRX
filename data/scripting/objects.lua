local raw = trxc.objects

local function make_properties(object_id)
  return setmetatable({ object_id = object_id }, {
    __index = function(self, key)
      if type(key) ~= "string" then
        return nil
      end
      return raw.get_property(self.object_id, key)
    end,
    __newindex = function(self, key, value)
      raw.set_property(self.object_id, key, value)
    end,
    __pairs = function(self)
      local property_names = raw.get_property_names(self.object_id)
      local i = 0
      return function()
        i = i + 1
        local name = property_names[i]
        if name == nil then
          return nil
        end
        return name, raw.get_property(self.object_id, name)
      end
    end,
  })
end

local Object = {}

Object.__index = function(self, key)
  if key == "properties" then
    return make_properties(self.object_id)
  end
  return nil
end

local objects = {
  swap_mesh = raw.swap_mesh,
}

trx.objects = setmetatable(objects, {
  Object = Object,
  __index = function(_, key)
    if type(key) == "number" then
      return setmetatable({ object_id = key }, Object)
    elseif type(key) == "string" then
      local object_id = trx.catalog.objects[key]
      if object_id ~= nil then
        return setmetatable({ object_id = object_id }, Object)
      end
    end
    return nil
  end,
})
