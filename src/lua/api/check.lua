-- What a value has to be to satisfy a declaration.
--
-- The registry next door says what is declared and where; this says what a
-- declaration accepts, and hands back the predicate that answers for it. The
-- two meet at a path: api.lua passes the lookup it owns to `reads_from`, and
-- nothing else crosses between them.
--
-- There are three ways a value is recognised. A handle is told by its
-- metatable, which is the C type name it was registered under. A value the
-- registry hands out is told by the class it carries, so a derived one counts
-- as the type it extends. A table a script writes out has no identity to go on
-- and is told by what it holds.

local M = {}

-- Where a name that is not a primitive is looked up. Set by api.lua, which owns
-- the declarations; this only asks what one is checked by.
local declared_check = function()
  return nil
end

function M.reads_from(lookup)
  declared_check = lookup
end

-- The eight names a declaration may write without declaring them. Every other
-- name is a path. Doc-facing names, not Lua ones.
local PRIMITIVES = {
  -- A whole number, which type() alone does not tell from a fractional one.
  integer = function(v)
    return math.type(v) == "integer"
  end,
  vec3 = function(v)
    return type(v) == "table" and v.x ~= nil and v.y ~= nil and v.z ~= nil
  end,
  any = function()
    return true
  end,
}

-- The rest are the Lua type of the same name.
for _, name in ipairs({ "number", "string", "boolean", "table", "function" }) do
  PRIMITIVES[name] = function(v)
    return type(v) == name
  end
end

-- A type this surface does not declare has nothing to check against, and what
-- a caller wrote goes through. Strict mode reaches for this where a name means
-- nothing to it; the seal reports the name itself.
M.anything = PRIMITIVES.any

function M.primitive(name)
  return PRIMITIVES[name]
end

-- Every type a spec accepts, in the order it declared them: one, or the several
-- a container key answers to - a number, and the name that reaches the same
-- thing.
function M.types_of(value)
  return type(value) == "table" and value or { value }
end

-- What a declaration accepts, as the reader writes it: one type, or the several
-- it answers to.
function M.label(declared)
  local names = {}
  for i, one in ipairs(M.types_of(declared)) do
    names[i] = tostring(one)
  end
  return #names == 0 and "any" or table.concat(names, " or ")
end

-- What a declaration accepts, as the messages below name it: what it is typed
-- by, or the keys it names where it is typed by nothing, and the many it holds
-- folded in.
function M.label_of(spec)
  local named = spec.type ~= nil and M.label(spec.type)
    or (spec.fields ~= nil and "table" or "any")
  return spec.list and ("list of " .. named) or named
end

-- A type answers to the path it was declared under. There is no second name to
-- keep in step with it: a declaration that means items.Item says items.Item,
-- and one that means something nobody declared fails the audit rather than
-- waving values through under a name that once meant something else.
local function by_name(name)
  return PRIMITIVES[name] or declared_check(name)
end

-- The predicate one value is checked by, or nil where this surface has no type
-- of that name. One that names several passes on any of them.
local function by_type(declared)
  if type(declared) ~= "table" then
    return by_name(declared)
  end
  local checks = {}
  for i, one in ipairs(declared) do
    checks[i] = by_name(one)
    if checks[i] == nil then
      return nil
    end
  end
  return function(value)
    for _, check in ipairs(checks) do
      if check(value) then
        return true
      end
    end
    return false
  end
end

-- A handle's metatable is its C type name - see LUA_Struct_Register - so one
-- type of handle is told from another. api.type registers one of these per
-- type, which is why Item and Room are absent from the primitives.
function M.by_metatable(backing)
  return function(v)
    return getmetatable(v) == backing
  end
end

-- A derived class carries the one it extends as its own metatable's __index,
-- which is what makes an inherited method reachable, so the chain to walk is
-- already there.
function M.by_identity(class)
  return function(v)
    local candidate = getmetatable(v)
    while candidate ~= nil do
      if candidate == class then
        return true
      end
      local meta = getmetatable(candidate)
      candidate = meta ~= nil and meta.__index or nil
    end
    return false
  end
end

-- The predicate a whole declaration is checked by. One that holds several of
-- something is checked as the list it is: a list of integers is a table of
-- integers, not an integer.
function M.of(spec)
  local one = by_type(spec.type)
  if one == nil or not spec.list then
    return one
  end
  return function(value)
    if type(value) ~= "table" then
      return false, "not a list"
    end
    for i, held in ipairs(value) do
      if not one(held) then
        return false, ("entry %d: expected %s"):format(i, M.label(spec.type))
      end
    end
    return true
  end
end

-- On the global, so a module reaches this one the way it reaches any other.
-- It is the registry's own and no part of the surface a script gets: reads_from
-- is a way into what strict mode checks against, so the seal takes it off again
-- as it takes trxc off.
trx.check = M

return M
