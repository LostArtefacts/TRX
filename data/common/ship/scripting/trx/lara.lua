local raw = trxc.lara

local lara = {}

-- Item proxy metatable
local getters = {
  exposure_bar = raw.get_exposure_bar,
}

local setters = {
  exposure_bar = raw.set_exposure_bar,
}

function lara.get_item()
  return trx.items[raw.get_item()]
end

local lara_mt = {
  __index = function(self, key)
    local getter = getters[key]
    return getter and getter() or nil
  end,
  __newindex = function(self, key, value)
    local setter = setters[key]
    if setter then
      setter(value)
      return
    end
    error("Cannot set field '" .. key .. "' on trx.lara", 2)
  end,
}

setmetatable(lara, lara_mt)
trx.lara = lara
