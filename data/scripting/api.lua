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
local namespaces = {}
local namespace_order = {}
-- module -> { name -> spec }. What the module's __index dispatches on.
local module_properties = {}
-- module -> function returning the handle it stands for.
local module_instances = {}
local strict_enabled = false
local sealed = false

local function split_path(path)
  local module, name = path:match("^([%w_]+)%.([%w_]+)$")
  assert(module ~= nil, "api path must be 'module.name', got: " .. tostring(path))
  return module, name
end

-- A path is 'module.name', or 'module.namespace.name' one level deeper.
-- Anything deeper is a sign the module wants splitting.
local function path_parts(path)
  local parts = {}
  for segment in tostring(path):gmatch("[^.]+") do
    parts[#parts + 1] = segment
  end
  assert(
    #parts == 2 or #parts == 3,
    "api path must be 'module.name' or 'module.namespace.name', got: " .. tostring(path)
  )
  return parts
end

-- The module, the table the member hangs off, and the member's own name.
local function resolve(path)
  local parts = path_parts(path)
  local module = parts[1]
  trx[module] = trx[module] or {}
  if #parts == 2 then
    return module, trx[module], parts[2]
  end
  local namespace = rawget(trx[module], parts[2])
  assert(
    type(namespace) == "table",
    "api: " .. path .. " needs api.namespace('" .. module .. "." .. parts[2] .. "') declared first"
  )
  return module, namespace, parts[3]
end

-- A module is a plain table, and api.define rawsets its functions straight into
-- it, so they are found before any of this runs. What this adds is the members a
-- table cannot hold: computed properties, and - for a module that stands for a
-- single C struct, as trx.lara stands for Lara - that struct's own fields.
--
-- The registry owns the metatable, so a member nobody declared is unreachable
-- rather than merely undocumented. That matters most here: neither a metatable
-- getter nor a struct field ever shows up in pairs(), so seal()'s audit cannot
-- see one.
local function install_module_meta(module)
  local props = module_properties[module]
  local instance = module_instances[module]
  if props == nil and instance == nil then
    return
  end

  trx[module] = trx[module] or {}
  setmetatable(trx[module], {
    __index = function(_, key)
      local prop = props ~= nil and props[key] or nil
      if prop ~= nil then
        return prop.get()
      end
      if instance ~= nil then
        local handle = instance()
        if handle ~= nil then
          return handle[key]
        end
      end
      return nil
    end,
    __newindex = function(_, key, value)
      local prop = props ~= nil and props[key] or nil
      if prop ~= nil then
        if prop.set == nil then
          error("trx." .. module .. "." .. tostring(key) .. " is read-only", 2)
        end
        prop.set(value)
        return
      end
      if instance ~= nil then
        local handle = instance()
        if handle ~= nil then
          -- The struct raises on a member it does not expose, which is what
          -- makes an undeclared field unreachable rather than silently dropped.
          handle[key] = value
          return
        end
      end
      error("Cannot set field '" .. tostring(key) .. "' on trx." .. module, 2)
    end,
  })
end

function api.module(name, spec)
  assert(type(name) == "string", "api.module: name must be a string")
  if modules[name] == nil then
    table.insert(module_order, name)
  end
  modules[name] = spec or {}

  -- A module that stands for one C struct reads and writes that struct's fields
  -- directly: trx.lara.air is Lara's air. `instance` hands back the handle.
  if modules[name].instance ~= nil then
    assert(type(modules[name].instance) == "function", "api.module: instance must be a function")
    module_instances[name] = modules[name].instance
    install_module_meta(name)
  end
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
  -- container path ("items", or "console.log") -> set of declared member names.
  local declared = {}
  local function mark(path)
    local parts = path_parts(path)
    local container = parts[1]
    local name = parts[2]
    if #parts == 3 then
      container = parts[1] .. "." .. parts[2]
      name = parts[3]
    end
    declared[container] = declared[container] or {}
    declared[container][name] = true
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
  -- The namespace table itself is a member of its module.
  for _, path in ipairs(namespace_order) do
    mark(path)
  end

  local undeclared = {}
  local function audit(container_path, tbl)
    for name in pairs(tbl or {}) do
      if not (declared[container_path] or {})[name] then
        table.insert(undeclared, "trx." .. container_path .. "." .. name)
      end
    end
  end

  for _, module in ipairs(module_order) do
    audit(module, trx[module])
  end
  -- A namespace hides its members one level down, where the module audit above
  -- cannot see them. Audit it as its own container.
  for _, path in ipairs(namespace_order) do
    local module, name = split_path(path)
    audit(path, rawget(trx[module] or {}, name))
  end

  if #undeclared > 0 then
    table.sort(undeclared)
    error(
      table.concat(undeclared, ", ")
        .. ": reachable from scripts but not declared, so the reference cannot describe it. "
        .. "Declare it with api.define, api.enum, api.const, api.property or api.namespace."
    )
  end

  sealed = true
end

-- Declares a grouping table on a module, holding related members under one
-- name. `call` makes the group itself callable, so calling the group works
-- alongside calling the members inside it.
--
-- A namespace has to be declared before anything inside it: it is the table the
-- members hang off, and seal() audits its contents just as it audits a module's.
function api.namespace(path, spec)
  assert(not sealed, "the trx.api registry is sealed; declarations happen at load time")
  assert(type(spec) == "table", "api.namespace: spec must be a table")
  local module, name = split_path(path)

  local namespace = {}
  if spec.call ~= nil then
    assert(type(spec.call) == "function", "api.namespace: call must be a function")
    setmetatable(namespace, {
      __call = function(_, ...)
        return spec.call(...)
      end,
    })
  end

  trx[module] = trx[module] or {}
  rawset(trx[module], name, namespace)

  if namespaces[path] == nil then
    table.insert(namespace_order, path)
  end
  namespaces[path] = spec
  return namespace
end

function api.define(path, spec)
  assert(not sealed, "the trx.api registry is sealed; declarations happen at load time")
  assert(type(spec) == "table", "api.define: spec must be a table")
  assert(type(spec.impl) == "function", "api.define: impl must be a function")
  local _, container, name = resolve(path)

  if registry[path] == nil then
    table.insert(order, path)
  end
  registry[path] = spec

  -- The raw implementation IS the public function. No wrapper, no overhead.
  -- rawset: some module tables guard __newindex, and a declaration is not a
  -- caller poking at the module.
  rawset(container, name, strict_enabled and make_checked(spec.impl, path, spec.params) or spec.impl)
  return spec.impl
end

-- Rebinds every registered function to (or away from) its checking wrapper.
function api.strict(enabled)
  strict_enabled = enabled and true or false
  for _, path in ipairs(order) do
    local _, container, name = resolve(path)
    local spec = registry[path]
    rawset(container, name, strict_enabled and make_checked(spec.impl, path, spec.params) or spec.impl)
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
-- The name a constant goes by in Lua. `strip` takes a prefix off the reflected
-- name: the C spelling is what the data files are keyed by and cannot move, but
-- trx.lara.ExtraMesh.EXTRA_MESH_OAR only says EXTRA_MESH twice.
function api.enum_name(spec, reflected_name)
  local strip = spec.strip
  if strip ~= nil and reflected_name:sub(1, #strip) == strip then
    return reflected_name:sub(#strip + 1)
  end
  return reflected_name
end

function api.enum(path, spec)
  assert(not sealed, "the trx.api registry is sealed; declarations happen at load time")
  assert(type(spec) == "table", "api.enum: spec must be a table")
  assert(type(spec.backing) == "string", "api.enum: backing must be a C enum name")
  local module, name = split_path(path)

  -- `bulk` is for a catalog-sized enum - every object in the game, say. It is
  -- described as a whole, and its constants carry no description apiece.
  local bulk = spec.bulk == true
  assert(bulk or type(spec.values) == "table", "api.enum: values must be a table")

  local public = {}
  local reflected = {}
  for _, constant in ipairs(enum.values(spec.backing)) do
    local value_name = api.enum_name(spec, constant.name)
    -- Stripping a prefix can collide two C constants onto one Lua name, and the
    -- second would quietly take the first one's place.
    assert(
      public[value_name] == nil,
      "api.enum: " .. path .. "." .. value_name .. " is the name of two constants of " .. spec.backing
    )
    if not bulk then
      assert(spec.values[value_name] ~= nil, "api.enum: " .. path .. "." .. value_name .. " is not documented")
    end
    public[value_name] = constant.value
    reflected[value_name] = true
  end

  for value_name in pairs(spec.values or {}) do
    assert(reflected[value_name], "api.enum: " .. path .. "." .. value_name .. " is not a constant of " .. spec.backing)
  end

  spec.count = 0
  for _ in pairs(public) do
    spec.count = spec.count + 1
  end

  -- The constants are held behind an empty table rather than in one. Lua only
  -- calls __newindex for a key the table does not have, so a value sitting in
  -- the table itself could be overwritten without the metatable seeing it, and
  -- an enum mirrors C: it is read-only.
  --
  -- __index also folds the case, so trx.catalog.objects.wolf and
  -- trx.catalog.objects.WOLF are the same constant. Upper case is canonical: it
  -- is what pairs() yields and what the docs list.
  local face = {}
  setmetatable(face, {
    __index = function(_, key)
      if type(key) ~= "string" then
        return nil
      end
      return public[key] ~= nil and public[key] or public[key:upper()]
    end,
    __newindex = function(_, key)
      error("trx." .. path .. "." .. tostring(key) .. ": an enum cannot be written to", 2)
    end,
    __pairs = function()
      return next, public, nil
    end,
  })

  trx[module] = trx[module] or {}
  rawset(trx[module], name, face)

  if enums[path] == nil then
    table.insert(enum_order, path)
  end
  enums[path] = spec
  return face
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

  local props = module_properties[module]
  if props == nil then
    props = {}
    module_properties[module] = props
  end
  props[name] = spec

  install_module_meta(module)
  return spec
end

function api.describe()
  local out = {
    modules = {},
    functions = {},
    types = {},
    enums = {},
    constants = {},
    properties = {},
    namespaces = {},
  }
  for _, name in ipairs(module_order) do
    table.insert(out.modules, {
      name = name,
      title = modules[name].title,
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
    local entry = {
      path = path,
      description = spec.description,
      examples = spec.examples,
      bulk = spec.bulk == true,
      count = spec.count,
      source = spec.source,
      values = {},
    }
    -- Names only, and no values: the ids are TRX's own, and a script refers to
    -- them by name.
    if entry.bulk then
      entry.names = {}
      for _, constant in ipairs(enum.values(spec.backing)) do
        table.insert(entry.names, api.enum_name(spec, constant.name))
      end
      table.sort(entry.names)
    end
    if not entry.bulk then
      for _, constant in ipairs(enum.values(spec.backing)) do
        local value_name = api.enum_name(spec, constant.name)
        table.insert(entry.values, {
          name = value_name,
          value = constant.value,
          description = spec.values[value_name],
        })
      end
      -- Numeric order: the order the constants are meant to be read in, and
      -- stable across dumps, which the reflected order is not.
      table.sort(entry.values, function(a, b)
        return a.value < b.value
      end)
    end
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
  for _, path in ipairs(namespace_order) do
    local spec = namespaces[path]
    table.insert(out.namespaces, {
      path = path,
      description = spec.description,
      params = spec.params,
      returns = spec.returns,
      examples = spec.examples,
      callable = spec.call ~= nil,
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

-- api.type reports members nobody exposed, so the logger must exist by the time
-- any module declares one, and modules load in source-list order, which puts log
-- after items. Force it here rather than depend on that ordering.
--
-- It has to come after `trx.api` is set, not before: log.lua declares itself
-- through the registry, so requiring it any earlier would hand it a half-built
-- api table.
require("trx.log")

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
