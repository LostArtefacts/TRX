-- Self-describing public API registry.
--
-- Every trx.* function declares its signature alongside its implementation. The
-- registry is what the docs are generated from, so the reference cannot drift
-- from the code.
--
-- PERFORMANCE: the spec is metadata, NOT a dispatch layer. `trx.items.spawn` is
-- the raw implementation - calling it costs exactly what calling any Lua
-- function costs. A generic spec-driven wrapper that validated arguments on
-- every call measured 26x slower (416ns vs 16ns), which is untenable for
-- anything a script calls per frame. Argument validation is therefore opt-in:
-- trx.api.strict(true) rebinds every registered function to a code-generated
-- checking wrapper (~108ns overhead), which builders can enable while
-- developing and leave off in play.

-- api.type reports members nobody exposed, so the logger must exist by then.
-- Module load order is not guaranteed; state the dependency.
require("trx.log")

-- Captured at module scope: the raw C bridge is removed from the globals once
-- the trx.* modules have loaded, so builders cannot reach past the public API.
local struct = trxc.struct
local enum = trxc.enum

local api = {}

-- path -> spec. Ordered separately so docs come out in declaration order.
local registry = {}
local order = {}
local modules = {}
local module_order = {}
local types = {}
local type_order = {}
local enums = {}
local enum_order = {}
local consts = {}
local const_order = {}
local properties = {}
local property_order = {}
-- module -> { name -> spec }. What the module's __index dispatches on.
local module_properties = {}
local strict_enabled = false
local sealed = false

local function split_path(path)
  local module, name = path:match("^([%w_]+)%.([%w_]+)$")
  assert(module ~= nil, "api path must be 'module.name', got: " .. tostring(path))
  return module, name
end

function api.module(name, spec)
  assert(type(name) == "string", "api.module: name must be a string")
  if modules[name] == nil then
    table.insert(module_order, name)
  end
  modules[name] = spec or {}
end

-- Builds a specialized validating wrapper by generating Lua source for this
-- exact parameter list. Avoids the per-call table.pack/unpack and spec loop that
-- made the generic approach 26x slower.
local function make_checked(fn, path, params)
  if params == nil or #params == 0 then
    return fn
  end

  local names, checks = {}, {}
  for i, p in ipairs(params) do
    names[i] = "a" .. i
    local fail = ("E(%q,%q)"):format(path, p.name)
    if p.optional then
      checks[#checks + 1] = ("if a%d==nil then a%d=D[%d] elseif not C[%d](a%d) then %s end"):format(i, i, i, i, i, fail)
    else
      checks[#checks + 1] = ("if not C[%d](a%d) then %s end"):format(i, i, fail)
    end
  end

  local src = ("local fn,C,D,E=... return function(%s) %s return fn(%s) end"):format(
    table.concat(names, ","),
    table.concat(checks, " "),
    table.concat(names, ",")
  )

  local checkers, defaults = {}, {}
  for i, p in ipairs(params) do
    checkers[i] = api.checkers[p.type] or function()
      return true
    end
    defaults[i] = p.default
  end

  local function on_fail(where, arg)
    error(("%s: invalid argument '%s'"):format(where, arg), 3)
  end
  return load(src, "=" .. path .. " (checked)")(fn, checkers, defaults, on_fail)
end

-- Type name -> predicate. Doc-facing type names, not Lua type names.
api.checkers = {
  integer = function(v)
    return math.type(v) == "integer"
  end,
  number = function(v)
    return type(v) == "number"
  end,
  string = function(v)
    return type(v) == "string"
  end,
  boolean = function(v)
    return type(v) == "boolean"
  end,
  table = function(v)
    return type(v) == "table"
  end,
  ["function"] = function(v)
    return type(v) == "function"
  end,
  vec3 = function(v)
    return type(v) == "table" and v.x ~= nil and v.y ~= nil and v.z ~= nil
  end,
  Item = function(v)
    return v ~= nil
  end,
  any = function()
    return true
  end,
}

-- Called once, from C, after the trx.* modules have loaded. Declarations are the
-- engine's to make; a level script re-opening the surface would defeat the point
-- of declaring it.
--
-- Also audits the finished surface: an assignment straight onto a module table
-- works for scripts, but the docs never see it. Refuse to boot instead.
function api.seal()
  local declared = {}
  local function mark(path)
    local module, name = split_path(path)
    declared[module] = declared[module] or {}
    declared[module][name] = true
  end
  for _, path in ipairs(order) do
    mark(path)
  end
  for _, path in ipairs(enum_order) do
    mark(path)
  end
  for _, path in ipairs(const_order) do
    mark(path)
  end

  for _, module in ipairs(module_order) do
    local undeclared = {}
    for name in pairs(trx[module] or {}) do
      if not (declared[module] or {})[name] then
        table.insert(undeclared, "trx." .. module .. "." .. name)
      end
    end
    if #undeclared > 0 then
      table.sort(undeclared)
      error(
        table.concat(undeclared, ", ")
          .. ": reachable from scripts but not declared, so the reference cannot describe it. "
          .. "Declare it with api.define, api.enum, api.const or api.property."
      )
    end
  end

  sealed = true
end

function api.define(path, spec)
  assert(not sealed, "the trx.api registry is sealed; declarations happen at load time")
  assert(type(spec) == "table", "api.define: spec must be a table")
  assert(type(spec.impl) == "function", "api.define: impl must be a function")
  local module, name = split_path(path)

  if registry[path] == nil then
    table.insert(order, path)
  end
  registry[path] = spec

  trx[module] = trx[module] or {}
  -- The raw implementation IS the public function. No wrapper, no overhead.
  -- rawset: some module tables guard __newindex, and a declaration is not a
  -- caller poking at the module.
  rawset(trx[module], name, strict_enabled and make_checked(spec.impl, path, spec.params) or spec.impl)
  return spec.impl
end

-- Rebinds every registered function to (or away from) its checking wrapper.
function api.strict(enabled)
  strict_enabled = enabled and true or false
  for _, path in ipairs(order) do
    local module, name = split_path(path)
    local spec = registry[path]
    rawset(trx[module], name, strict_enabled and make_checked(spec.impl, path, spec.params) or spec.impl)
  end
end

function api.is_strict()
  return strict_enabled
end

-- Returns the whole surface as plain data: what the docs generator consumes.
-- Declares a handle type: which C members are public, under what name, plus the
-- methods and computed members that complete the type.
--
-- Nothing is reachable from a handle until it is named here. The C table says
-- how to reach a member; this says whether it is part of the API and what it is
-- called. A member C can reach but nobody declares simply does not exist.
function api.type(path, spec)
  assert(not sealed, "the trx.api registry is sealed; declarations happen at load time")
  assert(type(spec) == "table", "api.type: spec must be a table")
  assert(type(spec.backing) == "string", "api.type: backing must be a C type name")
  local backing = spec.backing

  local declared = {}

  for name, field in pairs(spec.fields or {}) do
    local from = field.from or name
    local writable = field.writable ~= false
    struct.expose_field(backing, name, from, writable)
    declared[from] = true
  end

  for name, method in pairs(spec.methods or {}) do
    struct.expose_method(backing, name, method.from or name)
  end

  for name, computed in pairs(spec.extensions or {}) do
    assert(type(computed.impl) == "function", "api.type: extension '" .. name .. "' needs an impl")
    struct.expose_computed(backing, name, computed.impl)
  end

  -- Opt-in exposure is silent by nature: forgetting to declare a member means it
  -- quietly does not exist. Say so, rather than let it vanish unnoticed.
  local undeclared = {}
  for _, member in ipairs(struct.members(backing)) do
    if not declared[member.name] then
      table.insert(undeclared, member.name)
    end
  end
  if #undeclared > 0 then
    table.sort(undeclared)
    trx.log.debug(
      backing .. ": " .. #undeclared .. " member(s) not exposed to scripts: " .. table.concat(undeclared, ", ")
    )
  end

  if types[path] == nil then
    table.insert(type_order, path)
  end
  types[path] = spec
end

-- Declares an enum: what the constants of a C enum are called in Lua, and what
-- they mean. As with api.type, C is the mechanism and this is the contract - the
-- names and values are reflected out of ENUM_MAP (see trx/game/enum.c), so a
-- number is never written twice and the two cannot drift.
--
-- Unlike a struct, an enum is small and wholly public: there is nothing to hide,
-- so exposure is not opt-in. Every constant must be documented, and documenting
-- one that does not exist is an error.
function api.enum(path, spec)
  assert(not sealed, "the trx.api registry is sealed; declarations happen at load time")
  assert(type(spec) == "table", "api.enum: spec must be a table")
  assert(type(spec.backing) == "string", "api.enum: backing must be a C enum name")
  assert(type(spec.values) == "table", "api.enum: values must be a table")
  local module, name = split_path(path)

  local public = {}
  local reflected = {}
  for _, constant in ipairs(enum.values(spec.backing)) do
    assert(spec.values[constant.name] ~= nil, "api.enum: " .. path .. "." .. constant.name .. " is not documented")
    public[constant.name] = constant.value
    reflected[constant.name] = true
  end

  for value_name in pairs(spec.values) do
    assert(reflected[value_name], "api.enum: " .. path .. "." .. value_name .. " is not a constant of " .. spec.backing)
  end

  trx[module] = trx[module] or {}
  rawset(trx[module], name, public)

  if enums[path] == nil then
    table.insert(enum_order, path)
  end
  enums[path] = spec
  return public
end

-- Declares a lone constant sitting on the module table - an angle unit is a macro,
-- not a C enum, so api.enum has nothing to reflect. The value still comes from C,
-- so naming one C does not export fails here rather than quietly being nil.
function api.const(path, spec)
  assert(not sealed, "the trx.api registry is sealed; declarations happen at load time")
  assert(type(spec) == "table", "api.const: spec must be a table")
  assert(spec.value ~= nil, "api.const: " .. path .. " has no value; is it exported from C?")
  local module, name = split_path(path)

  if consts[path] == nil then
    table.insert(const_order, path)
  end
  consts[path] = spec

  trx[module] = trx[module] or {}
  rawset(trx[module], name, spec.value)
  return spec.value
end

-- Declares a computed member on a module table. It is not a function and not a
-- stored field: reading it calls into C, and writing it calls a setter, or fails
-- if there is none.
--
-- The registry OWNS the module's __index/__newindex, so an undeclared property is
-- unreachable rather than merely undocumented. That matters here more than
-- anywhere else: a metatable getter never appears in pairs(), so seal()'s audit
-- cannot see one, and a hand-rolled getter table would sail past it.
function api.property(path, spec)
  assert(not sealed, "the trx.api registry is sealed; declarations happen at load time")
  assert(type(spec) == "table", "api.property: spec must be a table")
  assert(type(spec.get) == "function", "api.property: get must be a function")
  assert(spec.set == nil or type(spec.set) == "function", "api.property: set must be a function")
  local module, name = split_path(path)

  if properties[path] == nil then
    table.insert(property_order, path)
  end
  properties[path] = spec

  trx[module] = trx[module] or {}
  local props = module_properties[module]
  if props == nil then
    props = {}
    module_properties[module] = props
    -- api.define uses rawset, so declared functions sit in the table itself and
    -- are found before __index ever runs.
    setmetatable(trx[module], {
      __index = function(_, key)
        local prop = props[key]
        if prop == nil then
          return nil
        end
        return prop.get()
      end,
      __newindex = function(_, key, value)
        local prop = props[key]
        if prop == nil then
          error("Cannot set field '" .. key .. "' on trx." .. module, 2)
        end
        if prop.set == nil then
          error("trx." .. module .. "." .. tostring(key) .. " is read-only", 2)
        end
        prop.set(value)
      end,
    })
  end
  props[name] = spec
  return spec
end

function api.describe()
  local out = { modules = {}, functions = {}, types = {}, enums = {}, constants = {}, properties = {} }
  for _, name in ipairs(module_order) do
    table.insert(out.modules, {
      name = name,
      description = modules[name].description,
      order = modules[name].order,
    })
  end
  for _, path in ipairs(order) do
    local spec = registry[path]
    table.insert(out.functions, {
      path = path,
      description = spec.description,
      params = spec.params,
      returns = spec.returns,
      examples = spec.examples,
    })
  end
  for _, path in ipairs(type_order) do
    local spec = types[path]
    local entry = {
      path = path,
      description = spec.description,
      fields = {},
      methods = {},
      extensions = {},
    }
    for name, field in pairs(spec.fields or {}) do
      table.insert(entry.fields, {
        name = name,
        type = field.type,
        writable = field.writable ~= false,
        description = field.description,
        enum = field.enum,
      })
    end
    for name, method in pairs(spec.methods or {}) do
      table.insert(entry.methods, {
        name = name,
        description = method.description,
        params = method.params,
        returns = method.returns,
        examples = method.examples,
      })
    end
    for name, computed in pairs(spec.extensions or {}) do
      table.insert(entry.extensions, {
        name = name,
        type = computed.type,
        description = computed.description,
      })
    end
    local by_name = function(a, b)
      return a.name < b.name
    end
    table.sort(entry.fields, by_name)
    table.sort(entry.methods, by_name)
    table.sort(entry.extensions, by_name)
    table.insert(out.types, entry)
  end
  for _, path in ipairs(enum_order) do
    local spec = enums[path]
    local entry = { path = path, description = spec.description, values = {} }
    for _, constant in ipairs(enum.values(spec.backing)) do
      table.insert(entry.values, {
        name = constant.name,
        value = constant.value,
        description = spec.values[constant.name],
      })
    end
    -- Numeric order: the order the constants are meant to be read in, and stable
    -- across dumps, which the reflected order is not.
    table.sort(entry.values, function(a, b)
      return a.value < b.value
    end)
    table.insert(out.enums, entry)
  end
  for _, path in ipairs(const_order) do
    local spec = consts[path]
    table.insert(out.constants, {
      path = path,
      value = spec.value,
      description = spec.description,
    })
  end
  for _, path in ipairs(property_order) do
    local spec = properties[path]
    table.insert(out.properties, {
      path = path,
      type = spec.type,
      description = spec.description,
      enum = spec.enum,
      writable = spec.set ~= nil,
    })
  end
  return out
end

trx.api = api

-- Minimal JSON encoder. The dump is consumed by tools/update_lua_docs; keeping
-- the encoding here means the whole API surface - C field tables included - is
-- serialized from one place.
local function encode(value, out)
  local kind = type(value)
  if value == nil then
    out[#out + 1] = "null"
  elseif kind == "boolean" then
    out[#out + 1] = tostring(value)
  elseif kind == "number" then
    out[#out + 1] = tostring(value)
  elseif kind == "string" then
    out[#out + 1] = '"' .. value:gsub("\\", "\\\\"):gsub('"', '\\"'):gsub("\n", "\\n"):gsub("\t", "\\t") .. '"'
  elseif kind == "table" then
    if value[1] ~= nil or next(value) == nil then
      out[#out + 1] = "["
      for i, v in ipairs(value) do
        if i > 1 then
          out[#out + 1] = ","
        end
        encode(v, out)
      end
      out[#out + 1] = "]"
    else
      local keys = {}
      for k in pairs(value) do
        keys[#keys + 1] = k
      end
      table.sort(keys)
      out[#out + 1] = "{"
      for i, k in ipairs(keys) do
        if i > 1 then
          out[#out + 1] = ","
        end
        encode(tostring(k), out)
        out[#out + 1] = ":"
        encode(value[k], out)
      end
      out[#out + 1] = "}"
    end
  end
  return out
end

-- The complete public API surface. One registry, one source: whatever is not
-- declared here is not reachable from a script, so the dump cannot omit anything
-- that exists.
function api.to_json()
  return table.concat(encode(api.describe(), {}))
end
