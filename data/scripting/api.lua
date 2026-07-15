-- Self-describing public API registry.
--
-- Every trx.* function declares its signature alongside its implementation. The
-- registry is what the docs are generated from, so the reference cannot drift
-- from the code.
--
-- On performance: the spec is metadata, not a dispatch layer.
-- `trx.items.spawn` is the raw implementation - calling it costs exactly what
-- calling any Lua function costs. A generic spec-driven wrapper that validated
-- arguments on every call measured 26x slower (416ns vs 16ns), which is
-- untenable for anything a script calls per frame. Argument validation is
-- therefore opt-in: trx.api.strict(true) rebinds every registered function to
-- a code-generated checking wrapper (~108ns overhead), which builders can
-- enable while developing and leave off in play.

-- Captured at module scope: the raw C bridge is removed from the globals once
-- the trx.* modules have loaded, so builders cannot reach past the public API.
local struct = trxc.struct
local enum = trxc.enum
local capi = trxc.api
-- Hardening nils this out of the globals, and api.strict runs long after that.
local load = load

local api = {}

-- A registry keyed by path, keeping the paths in declaration order alongside so
-- the docs come out the way the files declare, not the way pairs() happens to
-- iterate. record() is the only writer.
local function ordered()
  return { by_path = {}, order = {} }
end

local function record(reg, path, spec)
  if reg.by_path[path] == nil then
    reg.order[#reg.order + 1] = path
  end
  reg.by_path[path] = spec
  return spec
end

-- Every registry projects to a doc entry the same way: walk it in declaration
-- order, turn each spec into a plain table. Only the projection differs.
local function collect(reg, project)
  local list = {}
  for _, key in ipairs(reg.order) do
    list[#list + 1] = project(key, reg.by_path[key])
  end
  return list
end

local registry = ordered()
local modules = ordered()
local types = ordered()
local enums = ordered()
local consts = ordered()
local properties = ordered()
local namespaces = ordered()
-- path -> { fn = what calling the namespace runs }. Held apart from the
-- metatable so api.strict can swap the wrapper in.
local namespace_dispatch = {}
-- Type name -> predicate. Private: reachable, a script could hand strict mode a
-- checker that accepts anything.
local checkers
local containers = ordered()
-- module -> { get, count, accepts }. What indexing the module table reaches.
local module_containers = {}
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

-- The trx.<module> table, created on first mention. Every declaration hangs its
-- member off one of these.
local function module_table(module)
  trx[module] = trx[module] or {}
  return trx[module]
end

-- The guard every declarator opens with: declarations are the engine's to make
-- at load time, and a spec is a table. `fn` names the caller for the message.
local function opening(fn, spec)
  assert(not sealed, "the trx.api registry is sealed; declarations happen at load time")
  if spec ~= nil then
    assert(type(spec) == "table", fn .. ": spec must be a table")
  end
end

-- The module, the table the member hangs off, and the member's own name.
local function resolve(path)
  local parts = path_parts(path)
  local module = parts[1]
  module_table(module)
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
-- The registry owns the metatable, so an undeclared member is unreachable rather
-- than merely undocumented. That matters most here: neither a metatable getter
-- nor a struct field ever shows up in pairs(), so seal()'s audit cannot see one.
local function install_module_meta(module)
  local props = module_properties[module]
  local instance = module_instances[module]
  local container = module_containers[module]
  if props == nil and instance == nil and container == nil then
    return
  end

  module_table(module)
  local meta = {
    __index = function(_, key)
      local prop = props ~= nil and props[key] or nil
      if prop ~= nil then
        return prop.get()
      end
      if container ~= nil and container.accepts(key) then
        return container.get(key)
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
        -- A property is written, not called, so make_checked never sees it.
        -- Strict mode still has to.
        if strict_enabled and not checkers[prop.type](value) then
          error("trx." .. module .. "." .. tostring(key) .. ": expected a " .. tostring(prop.type), 2)
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
  }
  if container ~= nil and container.count ~= nil then
    meta.__len = function()
      return container.count()
    end
  end
  setmetatable(trx[module], meta)
end

function api.module(name, spec)
  opening("api.module")
  assert(type(name) == "string", "api.module: name must be a string")
  spec = record(modules, name, spec or {})

  -- A module that stands for one C struct reads and writes that struct's fields
  -- directly: trx.lara.air is Lara's air. `instance` hands back the handle.
  if spec.instance ~= nil then
    assert(type(spec.instance) == "function", "api.module: instance must be a function")
    module_instances[name] = spec.instance
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

  local variadic = params[#params].name == "..."
  local fixed = variadic and #params - 1 or #params

  local names, checks = {}, {}
  for i = 1, fixed do
    local p = params[i]
    names[i] = "a" .. i
    local fail = ("E(%q,%q)"):format(path, p.name)
    if p.optional then
      checks[#checks + 1] = ("if a%d==nil then a%d=D[%d] elseif not C[%d](a%d) then %s end"):format(i, i, i, i, i, fail)
    else
      checks[#checks + 1] = ("if not C[%d](a%d) then %s end"):format(i, i, fail)
    end
  end

  local sig = table.concat(names, ",")
  if variadic then
    sig = sig == "" and "..." or (sig .. ",...")
  end

  -- Several bridges read lua_gettop() to tell "not given" from "given nil", so an
  -- optional parameter with no default has to stay absent rather than arrive as
  -- nil. A variadic call carries its own count and needs none of this.
  local body = {}
  if not variadic then
    for i = fixed, 1, -1 do
      local p = params[i]
      if not (p.optional and p.default == nil) then
        break
      end
      table.insert(body, 1, ("if a%d==nil then return fn(%s) end"):format(i, table.concat(names, ",", 1, i - 1)))
    end
  end
  body[#body + 1] = ("return fn(%s)"):format(sig)

  local src = ("local fn,C,D,E=... return function(%s) %s %s end"):format(
    sig,
    table.concat(checks, " "),
    table.concat(body, " ")
  )

  local param_checkers, defaults = {}, {}
  for i = 1, fixed do
    local p = params[i]
    local check = checkers[p.type]
    assert(check ~= nil, path .. ": no checker for type '" .. tostring(p.type) .. "'")
    param_checkers[i] = check
    defaults[i] = p.default
  end

  local function on_fail(where, arg)
    error(("%s: invalid argument '%s'"):format(where, arg), 3)
  end
  -- Named as an API module is, so LUA_GetCallerInfo walks past this frame too.
  return load(src, "@trx/" .. path .. " (checked)")(fn, param_checkers, defaults, on_fail)
end

-- The function a caller reaches: the checking wrapper under strict mode, the raw
-- one otherwise. api.strict flips every binding through here.
local function bound(fn, path, params)
  return strict_enabled and make_checked(fn, path, params) or fn
end

-- Doc-facing type names, not Lua type names.
checkers = {
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
  any = function()
    return true
  end,
}

-- A handle's metatable is its C type name - see LUA_Struct_Register - so one
-- type of handle is told from another. api.type registers one of these per type,
-- which is why Item and Room are absent above.
local function handle_checker(backing)
  return function(v)
    return getmetatable(v) == backing
  end
end

-- Called once, from C, after the trx.* modules have loaded. Declarations are the
-- engine's to make; a level script re-opening the surface would defeat the point
-- of declaring it.
--
-- Also audits the finished surface: an assignment straight onto a module table
-- works for scripts, but the docs never see it. Refuse to boot instead.
function api.seal()
  -- The declaring half has done its job, and every one of those functions
  -- raises from here on. It goes the way trxc goes at this same moment: what a
  -- script cannot successfully call, it should not be able to reach. Done
  -- before the audit, so the audit sees the surface a script is left with. C
  -- keeps the two entrypoints it still needs - see lua/capi/api.c.
  trx.api = {
    strict = api.strict,
    is_strict = api.is_strict,
  }

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
  for _, path in ipairs(registry.order) do
    mark(path)
  end
  for _, path in ipairs(enums.order) do
    mark(path)
  end
  for _, path in ipairs(consts.order) do
    mark(path)
  end
  -- The namespace table itself is a member of its module.
  for _, path in ipairs(namespaces.order) do
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

  for _, module in ipairs(modules.order) do
    audit(module, trx[module])
  end
  -- A namespace hides its members one level down, where the module audit above
  -- cannot see them. Audit it as its own container.
  for _, path in ipairs(namespaces.order) do
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

  -- A type nothing can check prints a name that means nothing, and waves every
  -- value through while strict mode reports a clean run over it.
  local bad = {}
  local function audit_type(where, what, type_name)
    if checkers[type_name] == nil then
      table.insert(bad, where .. ": " .. what .. " has an unknown type '" .. tostring(type_name) .. "'")
      return false
    end
    return true
  end

  local function audit_params(where, params)
    local optional_seen = false
    for _, p in ipairs(params or {}) do
      if p.name ~= "..." then
        if audit_type(where, "'" .. tostring(p.name) .. "'", p.type) and p.default ~= nil then
          if not checkers[p.type](p.default) then
            table.insert(bad, where .. ": the default for '" .. p.name .. "' is not a " .. p.type)
          end
        end
        -- Nothing can reach it without passing the optional one too.
        if p.optional then
          optional_seen = true
        elseif optional_seen then
          table.insert(bad, where .. ": '" .. p.name .. "' is required but follows an optional parameter")
        end
      end
    end
  end

  for _, path in ipairs(registry.order) do
    audit_params(path, registry.by_path[path].params)
  end
  for _, path in ipairs(namespaces.order) do
    audit_params(path, namespaces.by_path[path].params)
  end
  -- Strict mode checks a method's arguments too, and make_checked needs a
  -- checker for each. Without this, a type nobody can check would only surface
  -- the day a builder turned strict mode on.
  for _, path in ipairs(types.order) do
    for name, method in pairs(types.by_path[path].methods or {}) do
      audit_params(path .. "." .. name, method.params)
    end
  end
  for _, path in ipairs(properties.order) do
    audit_type(path, "the property", properties.by_path[path].type)
  end

  if #bad > 0 then
    table.sort(bad)
    error(table.concat(bad, "\n"))
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
  opening("api.namespace", spec)
  local module, name = split_path(path)

  local namespace = {}
  if spec.call ~= nil then
    assert(type(spec.call) == "function", "api.namespace: call must be a function")
    -- Through a holder, so api.strict can swap the wrapper in as it does for the
    -- members.
    local dispatch = { fn = bound(spec.call, path, spec.params) }
    namespace_dispatch[path] = dispatch
    setmetatable(namespace, {
      __call = function(_, ...)
        return dispatch.fn(...)
      end,
    })
  end

  rawset(module_table(module), name, namespace)

  record(namespaces, path, spec)
  return namespace
end

-- Declares that a module table can be indexed - trx.items[3], trx.objects.wolf -
-- and, if it can be counted, that #trx.items is its length.
--
-- The registry owns the module's metatable, so a module that set its own would
-- lose it to the first property declared on it. And pairs() never sees a
-- metatable, so only a declaration puts the indexing in the reference.
function api.container(name, spec)
  opening("api.container", spec)
  assert(type(spec.get) == "function", "api.container: get must be a function")
  assert(spec.count == nil or type(spec.count) == "function", "api.container: count must be a function")
  assert(type(spec.key) == "table", "api.container: key must describe what the module is indexed by")

  -- A module keyed only by number leaves a string key to the rest of the
  -- metatable, so trx.rooms.nonsense is nil rather than an error out of C.
  local by_number_only = spec.key.type == "integer"
  module_containers[name] = {
    get = spec.get,
    count = spec.count,
    accepts = function(key)
      local kind = type(key)
      return kind == "number" or (kind == "string" and not by_number_only)
    end,
  }

  record(containers, name, spec)

  install_module_meta(name)
end

function api.define(path, spec)
  opening("api.define", spec)
  assert(type(spec.impl) == "function", "api.define: impl must be a function")
  local _, container, name = resolve(path)

  record(registry, path, spec)

  -- The raw implementation is the public function. No wrapper, no overhead.
  -- rawset: some module tables guard __newindex, and a declaration is not a
  -- caller poking at the module.
  rawset(container, name, bound(spec.impl, path, spec.params))
  return spec.impl
end

-- A method is called with the handle as its first argument, which the
-- declaration does not name because a script never writes it. Checking it
-- catches `item.distance_to(pos)` - a dot where a colon was meant.
local function method_params(spec_params, type_name)
  local params = { { name = "self", type = type_name } }
  for i, param in ipairs(spec_params or {}) do
    params[i + 1] = param
  end
  return params
end

-- A type's methods reach C directly, so make_checked has to go in front of the
-- C function rather than around a Lua one. Extensions take nothing but the
-- handle __index hands them, so there is nothing of the script's to check.
local function bind_type_methods(path)
  local spec = types.by_path[path]
  local backing = spec.backing
  local _, type_name = split_path(path)
  for name, method in pairs(spec.methods or {}) do
    local from = method.from or name
    -- The wrapper goes in front of the C function, which struct.method hands
    -- back; without strict mode the name reaches C to bind directly.
    local exposed = from
    if strict_enabled then
      exposed = make_checked(struct.method(backing, from), path .. "." .. name, method_params(method.params, type_name))
    end
    struct.expose_method(backing, name, exposed)
  end
end

-- Rebinds every registered function to (or away from) its checking wrapper.
function api.strict(enabled)
  strict_enabled = enabled and true or false
  for _, path in ipairs(registry.order) do
    local _, container, name = resolve(path)
    local spec = registry.by_path[path]
    rawset(container, name, bound(spec.impl, path, spec.params))
  end
  for path, dispatch in pairs(namespace_dispatch) do
    local spec = namespaces.by_path[path]
    dispatch.fn = bound(spec.call, path, spec.params)
  end
  for _, path in ipairs(types.order) do
    bind_type_methods(path)
  end
end

function api.is_strict()
  return strict_enabled
end

-- Declares a handle type: which C members are public, under what name, plus the
-- methods and computed members that complete the type.
--
-- Nothing is reachable from a handle until it is named here. The C table says
-- how to reach a member; this says whether it is part of the API and what it is
-- called. A member C can reach but nobody declares simply does not exist.
function api.type(path, spec)
  opening("api.type", spec)
  assert(type(spec.backing) == "string", "api.type: backing must be a C type name")
  local backing = spec.backing

  -- Declaring the type is what makes its name checkable in a params list.
  local _, type_name = split_path(path)
  checkers[type_name] = handle_checker(backing)

  local declared = {}

  for name, field in pairs(spec.fields or {}) do
    local from = field.from or name
    local writable = field.writable ~= false
    struct.expose_field(backing, name, from, writable)
    declared[from] = true
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

  record(types, path, spec)

  bind_type_methods(path)
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
  opening("api.enum", spec)
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
    -- pairs() hands the caller every value __pairs returns, so returning
    -- `next, public` would hand out the very table the face is there to keep
    -- behind __newindex. The iterator closes over it instead.
    __pairs = function(self)
      local key
      return function()
        local value
        key, value = next(public, key)
        return key, value
      end,
        self,
        nil
    end,
  })

  rawset(module_table(module), name, face)

  record(enums, path, spec)
  return face
end

-- Declares a lone constant sitting on the module table - an angle unit is a macro,
-- not a C enum, so api.enum has nothing to reflect. The value still comes from C,
-- so naming one C does not export fails here rather than quietly being nil.
function api.const(path, spec)
  opening("api.const", spec)
  assert(spec.value ~= nil, "api.const: " .. path .. " has no value; is it exported from C?")
  local module, name = split_path(path)

  record(consts, path, spec)

  rawset(module_table(module), name, spec.value)
  return spec.value
end

-- Declares a computed member on a module table. It is not a function and not a
-- stored field: reading it calls into C, and writing it calls a setter, or fails
-- if there is none. The registry owns the module's metatable - see
-- install_module_meta - so a hand-rolled getter table would sail past seal().
function api.property(path, spec)
  opening("api.property", spec)
  assert(type(spec.get) == "function", "api.property: get must be a function")
  assert(spec.set == nil or type(spec.set) == "function", "api.property: set must be a function")
  local module, name = split_path(path)

  record(properties, path, spec)

  local props = module_properties[module]
  if props == nil then
    props = {}
    module_properties[module] = props
  end
  props[name] = spec

  install_module_meta(module)
  return spec
end

-- The whole surface as plain data: what the docs generator consumes.
function api.describe()
  local out = { types = {}, enums = {} }
  out.containers = collect(containers, function(name, spec)
    return {
      module = name,
      description = spec.description,
      key = spec.key,
      value = spec.value,
      countable = spec.count ~= nil,
      examples = spec.examples,
    }
  end)
  out.modules = collect(modules, function(name, spec)
    return {
      name = name,
      title = spec.title,
      description = spec.description,
      order = spec.order,
    }
  end)
  out.functions = collect(registry, function(path, spec)
    return {
      path = path,
      description = spec.description,
      params = spec.params,
      returns = spec.returns,
      examples = spec.examples,
    }
  end)
  for _, path in ipairs(types.order) do
    local spec = types.by_path[path]
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
  for _, path in ipairs(enums.order) do
    local spec = enums.by_path[path]
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
  out.constants = collect(consts, function(path, spec)
    return {
      path = path,
      value = spec.value,
      description = spec.description,
    }
  end)
  out.namespaces = collect(namespaces, function(path, spec)
    return {
      path = path,
      description = spec.description,
      params = spec.params,
      returns = spec.returns,
      examples = spec.examples,
      callable = spec.call ~= nil,
    }
  end)
  out.properties = collect(properties, function(path, spec)
    return {
      path = path,
      type = spec.type,
      description = spec.description,
      enum = spec.enum,
      writable = spec.set ~= nil,
    }
  end)

  -- Which module a declaration lands under is the surface talking; which module
  -- loaded first is not - that is the source list in meson.build, and a module
  -- that requires another pulls it forward. The dump is committed and diffed, so
  -- it groups by module and cannot move when a load order does, the way a type's
  -- members and an enum's values already cannot.
  --
  -- Within a module the order is the order the file declares in, which is a
  -- choice someone made: trx.music reads play, pause, unpause, stop.
  local function by_module(list)
    local declared_at = {}
    for i, entry in ipairs(list) do
      declared_at[entry] = i
    end
    table.sort(list, function(a, b)
      local mod_a = a.path:match("^[^.]+")
      local mod_b = b.path:match("^[^.]+")
      if mod_a ~= mod_b then
        return mod_a < mod_b
      end
      return declared_at[a] < declared_at[b]
    end)
  end
  by_module(out.functions)
  by_module(out.types)
  by_module(out.enums)
  by_module(out.constants)
  by_module(out.namespaces)
  by_module(out.properties)
  -- Keyed by module, not by path, so by_module has nothing to sort on.
  table.sort(out.containers, function(a, b)
    return a.module < b.module
  end)
  table.sort(out.modules, function(a, b)
    return a.name < b.name
  end)
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

-- The registry declares itself, last of all. What is left of it once seal() has
-- run is what a script can use: strict mode, and the question of whether it is
-- on. The rest declared the surface and is gone by the time any script runs.
api.module("api", {
  order = 20,
  title = "API registry",
  description = "Argument checking for the whole of `trx`.",
})

api.define("api.strict", {
  description = "Turns argument checking on or off for every function in `trx`, and for the methods "
    .. "on its handles. Off by default: checking costs about 100ns a call, which a per-frame handler "
    .. "notices. Turn it on while writing a level, and leave it off in play.",
  params = { { name = "enabled", type = "boolean" } },
  examples = { [[trx.api.strict(true)]] },
  impl = api.strict,
})

api.define("api.is_strict", {
  description = "Whether argument checking is on.",
  returns = { type = "boolean" },
  impl = api.is_strict,
})

-- Sealing and dumping the surface stay C's to call, and the dump runs after the
-- seal has taken them off trx.api. Hand them over while they are still here.
capi.set_entrypoint("seal", api.seal)
capi.set_entrypoint("to_json", api.to_json)
