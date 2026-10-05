-- Builds the runtime side of a trx.* module: the tables, enums, handles,
-- classes, properties and collections a script reaches. What each member is
-- and does is written as annotations next to the code; this only installs what
-- a plain table cannot hold.
--
-- Which fields and properties take a write, and what each constant holds, are
-- recorded as they are installed, so the seal can hold the annotations to them.

local struct = trxc.struct
local enum = trxc.enum
local catalog = trxc.catalog

-- The table every module hangs off. C creates it; this declares it for the
-- annotations.
---@class trx
trx = trx

local M = {}

local reflected = {
  properties = {},
  fields = {},
  class_fields = {},
  constants = {},
}

-- What strict mode checks a property write and a collection key against. Both
-- are nil while checking is off.
---@type fun(path: string, value: any)?
M.write_check = nil
---@type fun(path: string, key: any)?
M.key_check = nil

-- Handles and Lua classes by the path their annotations declare them under,
-- and the callable namespaces, so strict mode can wrap what a script calls.
local handles = {}
local classes = {}
local callables = {}

local function complain(called)
  return function(ok, fmt, ...)
    if not ok then
      error(called .. ": " .. fmt:format(...), 0)
    end
  end
end

---@param name string
---@return table
function M.module(name)
  trx[name] = trx[name] or {}
  return trx[name]
end

-- Enums

-- The name a constant reads as in Lua. C already spells it without its prefix.
local function enum_name(reflected_name)
  return reflected_name:upper()
end

-- A read-only table over `lookup`, which answers a name in any case. `walk`
-- hands back the table of names to values that pairs() goes over.
local function enum_face(path, lookup, walk)
  return setmetatable({}, {
    __index = function(_, key)
      if type(key) ~= "string" then
        return nil
      end
      return lookup(key)
    end,
    __newindex = function(_, key)
      error(
        ("trx.%s.%s: an enum cannot be written to"):format(path, tostring(key)),
        2
      )
    end,
    -- pairs() hands the caller every value __pairs returns, so the iterator
    -- closes over the table rather than handing it out.
    __pairs = function(self)
      local held = walk()
      local key
      return function()
        local value
        key, value = next(held, key)
        return key, value
      end,
        self,
        nil
    end,
  })
end

local enum_needs = complain("helpers.enum")

-- Returns the constants of C enum `backing` as a read-only table. `docs` maps
-- each constant's Lua name to its description, and must name every constant
-- and nothing else. Without `docs` the enum is described as a whole, as one
-- running to hundreds of constants is.
---@generic T
---@param path string
---@param backing string
---@param docs? T
---@return T
function M.enum(path, backing, docs)
  enum_needs(
    docs == nil or type(docs) == "table",
    "%s: docs must be a table",
    path
  )
  local public = {}
  for _, constant in ipairs(enum.values(backing)) do
    local name = enum_name(constant.name)
    enum_needs(
      public[name] == nil,
      "%s.%s is the name of two constants of %s",
      path,
      name,
      backing
    )
    enum_needs(
      docs == nil or docs[name] ~= nil,
      "%s.%s is not documented",
      path,
      name
    )
    public[name] = constant.value
  end
  for name in pairs(docs or {}) do
    enum_needs(
      public[name] ~= nil,
      "%s.%s is not a constant of %s",
      path,
      name,
      backing
    )
  end

  return enum_face(path, function(key)
    return public[key] or public[key:upper()]
  end, function()
    return public
  end)
end

-- Returns the identities a catalog holds as a read-only table, by name. A mod
-- can add identities after this, so a name is asked of the catalog when it is
-- not among the ones it held to begin with.
---@param path string
---@param context integer
---@return table<string, integer>
function M.catalog(path, context)
  local public = {}
  for _, constant in ipairs(catalog.values(context)) do
    public[enum_name(constant.name)] = constant.value
  end

  return enum_face(path, function(key)
    local value = public[key] or public[key:upper()]
    if value ~= nil then
      return value
    end
    return catalog.from_key(context, key)
      or catalog.from_key(context, key:lower())
  end, function()
    local held = {}
    for _, constant in ipairs(catalog.values(context)) do
      held[enum_name(constant.name)] = constant.value
    end
    return held
  end)
end

-- Handles

local handle_needs = complain("helpers.handle")

-- Exposes the C struct `backing` as the handle type `path`. `fields` maps each
-- public name to the C member it reads, and only names in `writable` take a
-- write. Returns the table the type's methods are declared on.
--
-- Assigning a function to that table binds a method. A name C has a method
-- for binds the C method, so a method C implements is declared with an empty
-- body. `methods` maps a public name to a C method of another name, and `lua`
-- lists the names whose Lua body is the implementation even though C has a
-- method of the same name.
---@param path string
---@param backing string
---@param spec { fields?: table<string, string>, writable?: string[], extensions?: table<string, function>, methods?: table<string, string>, lua?: string[] }
---@return table
function M.handle(path, backing, spec)
  local writable = {}
  for _, name in ipairs(spec.writable or {}) do
    writable[name] = true
  end
  local fields = {}
  for name, from in pairs(spec.fields or {}) do
    struct.expose_field(backing, name, from, writable[name] == true)
    fields[name] = writable[name] == true
  end
  for name in pairs(writable) do
    handle_needs(fields[name] ~= nil, "%s.%s is not a field", path, name)
  end
  for name, impl in pairs(spec.extensions or {}) do
    handle_needs(
      type(impl) == "function",
      "%s.%s: an extension is a function",
      path,
      name
    )
    struct.expose_computed(backing, name, impl)
  end
  reflected.fields[path] = fields

  local renamed = spec.methods or {}
  local lua = {}
  for _, name in ipairs(spec.lua or {}) do
    lua[name] = true
  end

  local methods = {}
  handles[path] = { backing = backing, methods = methods }

  return setmetatable({}, {
    __newindex = function(decl, name, fn)
      handle_needs(
        type(fn) == "function",
        "%s.%s: a method is a function",
        path,
        name
      )
      local c_name = renamed[name] or name
      local impl = fn
      if not lua[name] and pcall(struct.method, backing, c_name) then
        impl = c_name
      end
      handle_needs(
        renamed[name] == nil or impl == c_name,
        "%s.%s: C has no method '%s'",
        path,
        name,
        c_name
      )
      methods[name] = impl
      struct.expose_method(backing, name, impl)
      rawset(decl, name, fn)
    end,
  })
end

-- The handles declared through M.handle, keyed by path, each with the C type it
-- stands for and its methods: a C method name, or the Lua function bound in its
-- place.
function M.handles()
  return handles
end

-- Classes

local class_needs = complain("helpers.class")

-- Returns the class of a type written in Lua. Its methods are assigned to the
-- class after this. `extends` names the type it inherits methods, fields and
-- operators from. `fields` maps a field to its accessors: reading one calls
-- `get(self)`, and a field takes a write only where it has a `set`. A class
-- with accessors raises on a write to any other name. `operators` maps a
-- metamethod name without its underscores to its function.
---@param path string
---@param spec? { extends?: string, fields?: table<string, { get: function, set?: function }>, operators?: table<string, function> }
---@return table
function M.class(path, spec)
  spec = spec or {}
  local class = {}
  class.__index = class

  local getters, setters = {}, {}
  for name, field in pairs(spec.fields or {}) do
    class_needs(
      type(field.get) == "function",
      "%s.%s: get must be a function",
      path,
      name
    )
    getters[name], setters[name] = field.get, field.set
  end

  if spec.extends ~= nil then
    local parent = classes[spec.extends]
    class_needs(parent ~= nil, "%s extends an undeclared type", path)
    setmetatable(class, { __index = parent.class })
    -- Lua looks a metamethod up raw, so a derived class carries its own copy
    -- of the operators it inherits.
    for key, value in pairs(parent.class) do
      if
        type(key) == "string"
        and key:sub(1, 2) == "__"
        and key ~= "__index"
      then
        rawset(class, key, value)
      end
    end
    for name, getter in pairs(parent.getters) do
      if getters[name] == nil then
        getters[name], setters[name] = getter, parent.setters[name]
      end
    end
  end

  for name, impl in pairs(spec.operators or {}) do
    class_needs(
      type(impl) == "function",
      "%s: operator '%s' is a function",
      path,
      name
    )
    rawset(class, "__" .. name, impl)
  end

  if next(getters) ~= nil then
    class.__index = function(self, key)
      local getter = getters[key]
      if getter ~= nil then
        return getter(self)
      end
      -- The metamethods sit on the class beside the methods, and are no
      -- member of the type.
      if type(key) == "string" and key:sub(1, 2) == "__" then
        return nil
      end
      return class[key]
    end
    class.__newindex = function(self, key, value)
      local setter = setters[key]
      if setter == nil then
        if getters[key] ~= nil then
          error(("%s.%s is read-only"):format(path, tostring(key)), 2)
        end
        error(("%s has no member '%s'"):format(path, tostring(key)), 2)
      end
      setter(self, value)
    end
  end

  local fields = {}
  for name in pairs(getters) do
    fields[name] = setters[name] ~= nil
  end
  reflected.class_fields[path] = fields
  classes[path] = { class = class, getters = getters, setters = setters }
  return class
end

-- The class of a type written in Lua, for a module that hands out a value of a
-- type another module declares.
---@param path string
---@return table
function M.class_of(path)
  local found = classes[path]
  class_needs(found ~= nil, "'%s' is not declared", path)
  return found.class
end

-- The classes declared through M.class, keyed by path.
function M.classes()
  return classes
end

-- Tables with computed members

-- The members each module and namespace table declares, from the signatures
-- the build generates out of the annotations. Read on the first assignment,
-- which happens as the modules load.
local declared

local function declares(path, key)
  if declared == nil then
    declared = {}
    local ok, signatures = pcall(require, "trx.internal.signatures")
    for owner, names in pairs(ok and signatures and signatures.members or {}) do
      declared[owner] = {}
      for _, name in ipairs(names) do
        declared[owner][name] = true
      end
    end
  end
  return declared[path] ~= nil and declared[path][key] == true
end

-- One metatable per table serves everything a plain table cannot hold: its
-- computed properties, the collection it is indexed as, the C struct it stands
-- for, and its call. Each helper below adds its part and reinstalls it.
local served = setmetatable({}, { __mode = "k" })

local function serve(tbl, path)
  local state = served[tbl]
  if state == nil then
    state = { path = path, props = {} }
    served[tbl] = state
  end
  return state
end

-- Walks a collection one handle at a time, from the index it counts from up to
-- how far its keys run, passing the gaps of a sparse one by.
local function walk(container, tbl)
  local first = container.base
  local last = first + (container.limit or container.count)() - 1
  return function(_, i)
    while i < last do
      i = i + 1
      local value = container.get(i)
      if value ~= nil then
        return i, value
      end
    end
    return nil
  end,
    tbl,
    first - 1
end

-- A module that stands for a handle passes its members through to it. A colon
-- call hands the module over as self and a dot call hands over nothing, so the
-- module table tells the two apart.
local function member_of(handle, tbl, key)
  if handle == nil then
    return nil
  end
  local member = handle[key]
  if type(member) ~= "function" then
    return member
  end
  return function(first, ...)
    if first == tbl then
      return member(handle, ...)
    end
    return member(handle, first, ...)
  end
end

local function install(tbl)
  local state = served[tbl]
  local props, container, instance =
    state.props, state.container, state.instance
  local meta = {
    __index = function(_, key)
      local prop = props[key]
      if prop ~= nil then
        return prop.get()
      end
      if container ~= nil and container.accepts(key) then
        if M.key_check ~= nil then
          M.key_check(state.path, key)
        end
        return container.get(key)
      end
      if instance ~= nil then
        return member_of(instance(), tbl, key)
      end
      return nil
    end,
    __newindex = function(_, key, value)
      local prop = props[key]
      if prop ~= nil then
        if prop.set == nil then
          error(("trx.%s.%s is read-only"):format(state.path, key), 2)
        end
        if M.write_check ~= nil then
          M.write_check(state.path .. "." .. key, value)
        end
        return prop.set(value)
      end
      -- A member the annotations declare is the module defining it.
      if declares(state.path, key) then
        rawset(tbl, key, value)
        return
      end
      if instance ~= nil then
        local handle = instance()
        if handle ~= nil then
          -- The struct raises on a member it does not expose.
          handle[key] = value
          return
        end
      end
      error(
        ("Cannot set field '%s' on trx.%s"):format(tostring(key), state.path),
        2
      )
    end,
  }
  if container ~= nil and container.count ~= nil then
    meta.__len = function()
      return container.count()
    end
    meta.__pairs = function(t)
      return walk(container, t)
    end
  end
  if state.call ~= nil then
    meta.__call = function(_, ...)
      return state.call(...)
    end
  end
  setmetatable(tbl, meta)
end

-- Makes the module `tbl`, at `path`, stand for the handle `instance()` returns:
-- reading or writing a member the module does not hold reaches the handle.
---@param tbl table
---@param path string
---@param instance fun(): any
function M.instance(tbl, path, instance)
  serve(tbl, path).instance = instance
  install(tbl)
end

local property_needs = complain("helpers.properties")

-- Adds computed members to `tbl`, the table at `path`. Each property reads
-- through `get`, and takes a write only where it has a `set`.
---@param tbl table
---@param path string
---@param props table<string, { get: function, set?: function }>
function M.properties(tbl, path, props)
  local state = serve(tbl, path)
  for name, prop in pairs(props) do
    property_needs(
      type(prop.get) == "function",
      "%s.%s: get must be a function",
      path,
      name
    )
    property_needs(
      prop.set == nil or type(prop.set) == "function",
      "%s.%s: set must be a function",
      path,
      name
    )
    state.props[name] = prop
    reflected.properties[path .. "." .. name] = prop.set ~= nil
  end
  install(tbl)
end

local container_needs = complain("helpers.container")

-- Makes a table indexed as a collection: `get(key)` reaches one element,
-- `count()` says how many there are, and `limit()` how far the keys run where
-- the collection is sparse. `base` is the first key. A collection keyed by a
-- name as well as a number sets `by_name`. The collection is `tbl` where given,
-- such as the module itself, and a new table otherwise.
---@param path string
---@param spec { get: function, count?: function, limit?: function, base: integer, by_name?: boolean }
---@param tbl? table
---@return table
function M.container(path, spec, tbl)
  container_needs(
    type(spec.get) == "function",
    "%s: get must be a function",
    path
  )
  container_needs(
    spec.base == 0 or spec.base == 1,
    "%s: base counts from 0 or from 1",
    path
  )
  container_needs(
    spec.limit == nil or spec.count ~= nil,
    "%s: a limit needs a count beside it",
    path
  )
  tbl = tbl or {}
  local state = serve(tbl, path)
  local by_name = spec.by_name == true
  state.container = {
    get = spec.get,
    count = spec.count,
    limit = spec.limit,
    base = spec.base,
    accepts = function(key)
      local kind = type(key)
      return kind == "number" or (kind == "string" and by_name)
    end,
  }
  install(tbl)
  return tbl
end

-- Returns a table that groups members one level down in a module. With `call`
-- the group itself can be called.
---@param path string
---@param call? function
---@return table
function M.namespace(path, call)
  local tbl = {}
  if call ~= nil then
    local state = serve(tbl, path)
    state.call = call
    callables[path] = state
    install(tbl)
  end
  return tbl
end

-- The callable namespaces, keyed by path, each holding its `call`.
function M.callables()
  return callables
end

-- Returns `value`, recording it as the constant at `path`.
---@generic T
---@param path string
---@param value T
---@return T
function M.const(path, value)
  reflected.constants[path] = value
  return value
end

-- Which fields and properties the modules built take a write, and the value
-- of each constant, keyed by path.
function M.reflected()
  return reflected
end

return M
