-- Self-describing public API registry.
--
-- Every trx.* function declares its signature alongside its implementation. The
-- registry is what the docs are generated from, so the reference cannot drift
-- from the code.
--
-- The spec is metadata, not a dispatch layer. `trx.items.spawn` is the raw
-- implementation, and calling it costs what calling any Lua function costs,
-- because with checking off nothing wraps it at all. Checking is opt-in
-- instead: api.strict rebinds every registered function to a closure that
-- checks what it was handed, and its own description says what that costs.

-- Captured at module scope: the raw C bridge is removed from the globals once
-- the trx.* modules have loaded, so builders cannot reach past the public API.
local struct = trxc.struct

local enum = trxc.enum

local capi = trxc.api

-- What a declaration accepts, and the predicate that answers for it. The
-- registry says what is declared where; check.lua says what satisfies one, and
-- reads the declarations back through the lookup below.
local check = require("trx.check")

local api = {}

-- The module's own state, in one place rather than scattered among the
-- functions that read it.

-- One registry, keyed by the path a declaration was made under and keeping the
-- paths in declaration order alongside, so the docs come out the way the files
-- declare rather than the way pairs() happens to iterate.
--
-- An entry says what kind of thing was declared and what the declaration said.
-- What the declaration went on to build is added to the same entry - the
-- predicate strict mode calls, the class a value carries, the table a module
-- stands for - so "what is at this path" is one lookup rather than a search
-- across a table per question.
local declarations = { by_path = {}, order = {} }

-- Every kind of declaration there is, in the order they were made, and the
-- same kinds keyed by the declarator that makes one. The dump walks the first;
-- each() reaches a kind through the second. A kind is the table itself
-- wherever one is named, so there is nothing to keep a spelling of it in step
-- with.
local KINDS = {}
local KIND_OF = {}

-- Whether declarations are still being made, and whether what a script
-- calls is the checking wrapper. Both are read by the declarators below.
local strict_enabled = false

local sealed = false

-- The registry, and what makes a declarator.

-- What a caller of this module got wrong, named by the entry point that caught
-- it and formatted only where there is something to say. Every complaint the
-- module makes goes through one of these, so none of them spells its own name
-- out for itself.
local function complain(called)
  return function(ok, fmt, ...)
    if not ok then
      error(called .. ": " .. fmt:format(...), 0)
    end
  end
end

-- The entry for a path, opened on first mention. A table can be reached before
-- anything declares it - a property names the table it hangs off, and a group
-- of properties needs no declaration of its own - so an entry may hold what was
-- built at a path before it holds what was declared there. Until a declarator
-- claims it, it has no kind, and each() passes it by.
local function open(path)
  local entry = declarations.by_path[path]
  if entry == nil then
    entry = { path = path }
    declarations.by_path[path] = entry
    declarations.order[#declarations.order + 1] = path
  end
  return entry
end

local function at(path)
  return declarations.by_path[path]
end

-- What checking asks of the registry: the predicate the declaration at a path
-- built, and nothing else.
check.reads_from(function(path)
  local entry = declarations.by_path[path]
  return entry ~= nil and entry.check or nil
end)

-- Which module a declaration lands under is the surface talking; which module
-- loaded first is not - that is the source list in meson.build, and a module
-- that requires another pulls it forward. The dump is committed and diffed, so
-- it groups by module and cannot move when a load order does, the way a type's
-- members and an enum's values already cannot.
--
-- Within a module the order is the order the file declares in, which is a
-- choice someone made: trx.music reads play, pause, unpause, stop.
--
-- The field the module is read out of is named rather than assumed, so a kind
-- that carries its path under some other name says so.
local function by_module(field)
  return function(list)
    local declared_at = {}
    for i, entry in ipairs(list) do
      declared_at[entry] = i
    end
    table.sort(list, function(a, b)
      local mod_a = a[field]:match("^[^.]+")
      local mod_b = b[field]:match("^[^.]+")
      if mod_a ~= mod_b then
        return mod_a < mod_b
      end
      return declared_at[a] < declared_at[b]
    end)
  end
end

-- A module and a container carry no path to group on, so they read
-- alphabetically. A module may hand out several collections, so the member
-- settles the order between them and the dump cannot move when a declaration
-- somewhere else is added.
local function alphabetically(...)
  local fields = { ... }
  return function(list)
    table.sort(list, function(a, b)
      for _, field in ipairs(fields) do
        local x, y = a[field] or "", b[field] or ""
        if x ~= y then
          return x < y
        end
      end
      return false
    end)
  end
end

-- Every public declarator is made here, so what they share is written once:
-- declarations are the engine's to make at load time, a spec is a table, and
-- every one of them opens an entry under the path it was given. What is left
-- is the part that differs, which the kind supplies as `declare`.
--
-- A kind also says where it lands in the dump, how one entry reads there, and
-- how that list is ordered, so everything about a kind of declaration is in
-- one call.
local function declarator(name, key, spec)
  local kind = {
    key = key,
    describe = spec.describe,
    sort = spec.sort or by_module("path"),
  }
  local need = complain("api." .. name)

  -- What a second declaration at one path is told, settled here so nothing is
  -- built for the declarations that are fine.
  local taken = "%s is already declared."
    .. (spec.taken ~= nil and (" " .. spec.taken) or "")

  local function declare(path, declaration)
    need(
      not sealed,
      "the registry is sealed; declarations happen at load time"
    )
    need(type(path) == "string", "path must be a string")
    declaration = declaration or {}
    need(type(declaration) == "table", "spec must be a table")
    -- Any declaration may be deprecated: `true` says only that it is, and a
    -- string says what to reach for instead. It is the docs' to report; nothing
    -- stops working, which is the point of deprecating rather than removing.
    local deprecated = declaration.deprecated
    need(
      deprecated == nil or deprecated == true or type(deprecated) == "string",
      "deprecated must be true, or what to use instead"
    )
    -- A container is declared as the module it indexes and sits under the name
    -- the reference gives it, so a kind may name its own path.
    local entry = open(spec.path and spec.path(path) or path)
    need(entry.kind == nil, taken, path)
    entry.kind = kind
    entry.spec = declaration
    return spec.declare(entry, path, declaration, need)
  end
  KINDS[#KINDS + 1] = kind
  KIND_OF[declare] = kind
  return declare
end

-- Every declaration of one kind, in the order the files made them.
local function entries_of(kind)
  local list = {}
  for _, path in ipairs(declarations.order) do
    local entry = declarations.by_path[path]
    if entry.kind == kind then
      list[#list + 1] = entry
    end
  end
  return list
end

-- The same, reached by the declarator that makes one rather than by the kind
-- itself, which is the module's own and not something a caller here holds. The
-- dump walks KINDS and already has one.
local function each(maker)
  return entries_of(KIND_OF[maker])
end

-- Whether one entry is of a kind, named the same way.
local function made_by(entry, maker)
  return entry ~= nil and entry.kind == KIND_OF[maker]
end

-- What a declaration reads like once it is doc-facing.

-- A spec as the docs read it: a copy, with what it is typed by written out as
-- the path that names it, so the docs can link to the declaration and a
-- description several places share is still written once.
local function as_doc(spec)
  if type(spec) ~= "table" then
    return spec
  end
  local out = {}
  for key, value in pairs(spec) do
    out[key] = value
  end
  if out.type ~= nil then
    local accepted = check.types_of(out.type)
    out.type = #accepted == 1 and accepted[1] or accepted
  end
  -- A table argument or result declares the keys it is made of, and each of
  -- them reads like any other spec.
  for _, key in ipairs({ "params", "fields" }) do
    if type(out[key]) == "table" then
      local list = {}
      for i, nested in ipairs(out[key]) do
        list[i] = as_doc(nested)
      end
      out[key] = list
    end
  end
  return out
end

-- A type's members are declared in a table keyed by name, which pairs() walks
-- in any order, so each kind of member comes out sorted by name.
local function members(declared, read)
  local list = {}
  for name, member in pairs(declared or {}) do
    local one = read(member)
    one.name = name
    one.deprecated = member.deprecated
    list[#list + 1] = one
  end
  table.sort(list, function(a, b)
    return a.name < b.name
  end)
  return list
end

-- Params are always a list; what a call hands back is one spec or a list of
-- them, and a lone one is told apart by carrying a type of its own.
local function as_doc_list(list)
  if list == nil or list.type ~= nil then
    return as_doc(list)
  end
  local out = {}
  for i, spec in ipairs(list) do
    out[i] = as_doc(spec)
  end
  return out
end

-- Where a path lands, which every declaration asks in one of two ways: what a
-- path is made of, and what table it names.

local function segments(path)
  local parts = {}
  for segment in tostring(path):gmatch("[^.]+") do
    parts[#parts + 1] = segment
  end
  return parts
end

-- A path taken apart, which every declarator keeps on its entry as `where`.
-- `module` is the module it belongs to, `name` the member's own name, and
-- `owner` the path of the table that member hangs off - the module, or the
-- namespace inside it. A path is 'module.name', or 'module.namespace.name' where
-- `deep` allows one; anything further is a sign the module wants splitting.
local function placed(path, need, deep)
  local parts = segments(path)
  local count = #parts
  need(
    count == 2 or (deep and count == 3),
    deep and "path must be 'module.name' or 'module.namespace.name', got: %s"
      or "path must be 'module.name', got: %s",
    path
  )
  return {
    module = parts[1],
    namespace = count == 3 and parts[2] or nil,
    name = parts[count],
    owner = count == 3 and (parts[1] .. "." .. parts[2]) or parts[1],
  }
end

-- The trx.<module> table, created on first mention. Every declaration hangs its
-- member off one of these.
local function module_table(module)
  trx[module] = trx[module] or {}
  return trx[module]
end

-- The table a member hangs off: the module's own, or a namespace's inside it.
local function table_of(where, need)
  local tbl = module_table(where.module)
  if where.namespace == nil then
    return tbl
  end
  local namespace = rawget(tbl, where.namespace)
  need(
    type(namespace) == "table",
    "%s.%s.%s needs api.namespace('%s.%s') declared first",
    where.module,
    where.namespace,
    where.name,
    where.module,
    where.namespace
  )
  return namespace
end

-- What a declaration puts on the table a script reaches.

-- What a declaration is worth asking once and keeping, kept on the entry that
-- was asked about. Asked the first time something needs the answer rather than
-- as the declaration is made: a declaration may name a type its own file
-- declares further down, and reading it while the file is still being read
-- would answer nil.
local function once(holder, slot, answer, declared)
  local held = holder[slot]
  if held == nil then
    held = answer(declared)
    holder[slot] = held
  end
  return held
end

-- What checks a value against a declaration, waving it through where nothing
-- can: a type this surface does not declare is caught by the seal and by the
-- dump rather than here.
local function checker(declared)
  return check.of(declared) or check.anything
end

-- What indexing counts from, which is the base of the number the key names: a
-- container keyed by an item number counts as items do. A key that names no
-- number, or one with nothing to say, counts from zero.
local function base_of(key)
  for _, one in ipairs(check.types_of(key.type)) do
    local entry = at(one)
    if made_by(entry, api.number) and entry.spec.base ~= nil then
      return entry.spec.base
    end
  end
  return 0
end

-- What a module standing for a handle reaches on it. A method wants the handle
-- as its self, and the module is not one, so the handle stands in. A colon call
-- hands the module over as self and a dot call hands over nothing, so the module
-- table itself is what tells the two apart - no argument of a script's can be it.
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

-- A property is written, not called, so make_checked never sees it. Strict mode
-- still has to. Three frames up is whoever wrote it.
local function write_property(where, prop, value)
  local spec = prop.spec
  if spec.set == nil then
    error(where .. " is read-only", 3)
  end
  if strict_enabled then
    local ok, why = once(prop, "write_check", checker, spec)(value)
    if not ok then
      error(
        ("%s: %s"):format(where, why or "expected " .. check.label_of(spec)),
        3
      )
    end
  end
  spec.set(value)
end

-- What a script may index a collection with. The key declares a type and the
-- reference documents it, so strict mode holds a script to it: `trx.rooms[1.5]`
-- says so here rather than reaching C with a number no room answers to.
local function key_accepted(container, key)
  return once(container, "key_check", checker, container.spec.key)(key)
end

-- pairs() over the module walks the collection one handle at a time, so a
-- script iterates without the #..-1 idiom. It walks from whichever index the
-- collection counts from: the items and the rooms from zero, matching the
-- numbers level editors show, and a plain list from one. seal()'s audit reads
-- the module's own members instead, and iterates raw to reach them.
local function walk(container, tbl)
  local first = once(container, "base", base_of, container.spec.key)
  -- How far the keys run, which is not how many there are where a collection
  -- is sparse: the samples a level carries are a hundred keys spread over
  -- twice as many numbers. A dense one says nothing and the two agree.
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

-- A module is a plain table, and api.define rawsets its functions straight into
-- it, so they are found before any of this runs. What this adds is the members
-- a table cannot hold: computed properties, and - for a module that stands for
-- a single C struct, as trx.lara stands for Lara - that struct's own fields.
--
-- The registry owns the metatable, so an undeclared member is unreachable
-- rather than merely undocumented. That matters most here: neither a metatable
-- getter nor a struct field ever shows up in pairs(), so seal()'s audit cannot
-- see one.
--
-- Everything the metatable serves is on the entry: the properties hung off it,
-- the handle it stands for, the collection it is indexed as. Its path is what
-- the errors name, and `entry.table` is what the metatable goes on - trx.<name>
-- for a module, the namespace's own table for a namespace. Only a module can
-- carry an instance or be a container, so a namespace's entry has neither.
--
-- Called by whichever declaration adds one of the three, and again by the next,
-- so the metatable it leaves serves everything the entry holds by then.
local NO_PROPERTIES = {}

local function install_meta(entry)
  local owner, tbl = entry.path, entry.table
  local props = entry.properties or NO_PROPERTIES
  local instance, container = entry.instance, entry.container

  local meta = {
    __index = function(_, key)
      local prop = props[key]
      if prop ~= nil then
        return prop.spec.get()
      end
      if container ~= nil and container.accepts(key) then
        if strict_enabled and not key_accepted(container, key) then
          error(
            ("trx.%s[%s]: expected %s"):format(
              owner,
              tostring(key),
              check.label_of(container.spec.key)
            ),
            2
          )
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
        return write_property(
          ("trx.%s.%s"):format(owner, tostring(key)),
          prop,
          value
        )
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
      error(
        ("Cannot set field '%s' on trx.%s"):format(tostring(key), owner),
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
  -- A namespace declared callable already carries a metatable, and its call
  -- is part of the surface just as its members are.
  local existing = getmetatable(tbl)
  if existing ~= nil and existing.__call ~= nil then
    meta.__call = existing.__call
  end
  setmetatable(tbl, meta)
end

-- Argument checking, which is off until something turns it on.

-- Strict mode wraps the implementation in a closure that checks what it was
-- handed. `select` reads the arguments without copying them, and handing `...`
-- straight on keeps the count the caller gave: several bridges read
-- lua_gettop() to tell "not given" from "given nil", so an optional argument
-- nobody passed has to stay absent rather than arrive as nil.
local function make_checked(fn, path, params)
  if params == nil or #params == 0 then
    return fn
  end

  local variadic = params[#params].name == "..."
  local fixed = variadic and #params - 1 or #params

  local names, optional, predicates, defaults, labels = {}, {}, {}, {}, {}
  local fills_a_default = false
  for i = 1, fixed do
    local p = params[i]
    names[i] = p.name
    optional[i] = p.optional == true
    predicates[i] = checker(p)
    defaults[i] = p.default
    -- What the argument had to be, for a predicate that answers yes or no and
    -- has nothing of its own to say: a handle, a class, and every primitive.
    labels[i] = "expected " .. check.label_of(p)
    if p.default ~= nil then
      fills_a_default = true
    end
  end

  -- Three frames down to the caller: here, the wrapper, and whoever called it.
  local function checked(i, value)
    if value == nil and optional[i] then
      return
    end
    local ok, why = predicates[i](value)
    if not ok then
      error(
        ("%s: invalid argument '%s' - %s"):format(
          path,
          names[i],
          why or labels[i]
        ),
        3
      )
    end
  end

  if not fills_a_default then
    return function(...)
      for i = 1, fixed do
        checked(i, (select(i, ...)))
      end
      return fn(...)
    end
  end

  -- Substituting a default hands the implementation a different argument list
  -- than the caller wrote, so this one has to build one. Twelve of the
  -- surface's parameters declare a default; the rest take the path above.
  return function(...)
    local args = table.pack(...)
    for i = 1, fixed do
      if args[i] == nil and defaults[i] ~= nil then
        args[i] = defaults[i]
        if args.n < i then
          args.n = i
        end
      else
        checked(i, args[i])
      end
    end
    return fn(table.unpack(args, 1, args.n))
  end
end

-- The function a caller reaches: the checking wrapper under strict mode, the
-- raw one otherwise. api.strict flips every binding through here.
local function bound(fn, path, params)
  return strict_enabled and make_checked(fn, path, params) or fn
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
-- C function rather than around a Lua one. A method may instead carry an
-- `impl`, a Lua function taking the handle first, and is exposed as it stands.
-- Extensions take nothing but the handle __index hands them, so there is
-- nothing of the script's to check.
--
-- What a declaration has to carry is settled where it is made, so this only
-- binds. api.strict comes back through here every time it is flipped.
local function bind_type_methods(entry)
  local path = entry.path
  local spec = entry.spec
  local backing = spec.backing
  local class = entry.class

  -- A type written in Lua keeps its methods in the class table, so binding one
  -- is an assignment and strict mode swaps the wrapper in the same way it does
  -- for a module's functions.
  if class ~= nil then
    for name, method in pairs(spec.methods or {}) do
      rawset(
        class,
        name,
        bound(
          method.impl,
          path .. "." .. name,
          method_params(method.params, path)
        )
      )
    end
    return
  end

  for name, method in pairs(spec.methods or {}) do
    -- The wrapper goes in front of the C function, which struct.method hands
    -- back; without strict mode the name reaches C to bind directly.
    local exposed = method.impl or method.from or name
    if strict_enabled then
      exposed = make_checked(
        method.impl or struct.method(backing, method.from or name),
        path .. "." .. name,
        method_params(method.params, path)
      )
    end
    struct.expose_method(backing, name, exposed)
  end
end

-- The public surface. Everything above is written once; everything below
-- declares one kind of thing and says how it reads in the reference.

api.module = declarator("module", "modules", {
  sort = alphabetically("name"),
  declare = function(entry, name, spec, need)
    entry.table = module_table(name)

    -- A module that stands for one C struct reads and writes that struct's fields
    -- directly: trx.lara.air is Lara's air. `instance` hands back the handle.
    if spec.instance ~= nil then
      need(type(spec.instance) == "function", "instance must be a function")
      need(
        type(spec.instance_type) == "string",
        "instance_type must name the type the module stands for"
      )
      entry.instance = spec.instance
      entry.instance_type = spec.instance_type
      install_meta(entry)
    end
  end,

  describe = function(entry)
    return {
      name = entry.path,
      title = entry.spec.title,
      description = entry.spec.description,
      order = entry.spec.order,
      -- What indexing the module reaches, where it stands for one thing:
      -- trx.lara.air is a member of lara.Lara, and a reference written the way
      -- a script writes it lands there.
      instance_type = entry.instance_type,
    }
  end,
})

-- Declares a grouping table on a module, holding related members under one
-- name. `call` makes the group itself callable, so calling the group works
-- alongside calling the members inside it.
--
-- A namespace has to be declared before anything inside it: it is the table
-- the members hang off, and seal() audits its contents just as it audits a
-- module's.
api.namespace = declarator("namespace", "namespaces", {
  taken = "Declare it before anything inside it.",
  declare = function(entry, path, spec, need)
    local where = placed(path, need)
    entry.where = where
    -- The table below stands for the namespace from here on, and would replace
    -- what stands at its path: the table a collection is read through, or
    -- the one a group of properties hangs off. Either goes on reading as nil,
    -- which seal() cannot see - it audits the members a table holds, and
    -- neither indexing nor a property is ever one of them.
    need(
      entry.table == nil,
      "%s already stands for a table - a collection, or a group of properties.",
      path
    )
    local namespace = {}
    entry.table = namespace
    if spec.call ~= nil then
      need(type(spec.call) == "function", "call must be a function")
      -- Read off the entry rather than closed over, so api.strict can swap the
      -- wrapper in as it does for the members.
      entry.call = bound(spec.call, path, spec.params)
      setmetatable(namespace, {
        __call = function(_, ...)
          return entry.call(...)
        end,
      })
    end

    rawset(module_table(where.module), where.name, namespace)

    return namespace
  end,

  describe = function(entry)
    local spec = entry.spec
    return {
      path = entry.path,
      description = spec.description,
      params = as_doc_list(spec.params),
      returns = as_doc_list(spec.returns),
      examples = spec.examples,
      callable = spec.call ~= nil,
      implicit = spec.implicit,
    }
  end,
})

-- Declares that a module table can be indexed - trx.items[3], trx.objects.wolf
-- - and, if it can be counted, that #trx.items is its length.
--
-- The registry owns the module's metatable, so a module that set its own would
-- lose it to the first property declared on it. And pairs() never sees a
-- metatable, so only a declaration puts the indexing in the reference.
api.container = declarator("container", "containers", {
  path = function(name)
    return name .. "[]"
  end,
  sort = alphabetically("module", "member"),
  declare = function(entry, name, spec, need)
    need(type(spec.get) == "function", "get must be a function")
    need(
      spec.count == nil or type(spec.count) == "function",
      "count must be a function"
    )
    need(
      spec.limit == nil or type(spec.limit) == "function",
      "limit must be a function"
    )
    need(
      spec.limit == nil or spec.count ~= nil,
      "a limit says how far the keys run, so it needs a count beside it"
    )
    need(
      type(spec.key) == "table",
      "key must describe what the module is indexed by"
    )
    -- Where a collection counts from is the key's, so that a container keyed by
    -- an item number counts as items do without saying so twice.
    need(spec.base == nil, "where it counts from belongs to the key")
    need(
      spec.key.base == nil,
      "where the keys count from belongs to the number they name"
    )

    -- What the key accepts: one type, or several where a thing answers to a name
    -- as well as to its number. A module keyed only by number leaves a string key
    -- to the rest of the metatable, so trx.rooms.nonsense is nil rather than an
    -- error out of C.
    local accepted = check.types_of(spec.key.type)
    local by_number_only = true
    for _, one in ipairs(accepted) do
      if one == "string" or one == "any" then
        by_number_only = false
      end
    end
    -- Declared under the name the reference already gives it - `trx.items[]` is
    -- what the page anchors - so being indexable is a declaration like any
    -- other, and the module it sits on points at it.
    local container = entry
    container.get = spec.get
    container.count = spec.count
    container.limit = spec.limit
    container.accepts = function(key)
      local kind = type(key)
      return kind == "number" or (kind == "string" and not by_number_only)
    end

    -- A collection is declared as the module that is indexed, or as the member
    -- of it the collection is read through: one segment or two, where every
    -- other declaration names a member and so has one more.
    local parts = segments(name)
    need(
      #parts == 1 or #parts == 2,
      "a collection is declared as 'module' or 'module.member', got: %s",
      name
    )
    container.module = parts[1]
    container.member = parts[2]

    -- The table the collection is indexed on: the module's own where the module
    -- is indexed, and a member of it where the module hands a collection out.
    -- Either way the entry at that path owns the table and its metatable, as it
    -- does for the properties declared on one, so a later declaration there
    -- finds the indexing rather than replacing it.
    local owner = open(name)
    if container.member == nil then
      owner.table = module_table(name)
    else
      owner.table = owner.table or {}
      rawset(module_table(container.module), container.member, owner.table)
    end
    owner.container = container
    install_meta(owner)
    return owner.table
  end,

  describe = function(entry)
    return {
      module = entry.module,
      member = entry.member,
      description = entry.spec.description,
      key = as_doc(entry.spec.key),
      value = as_doc(entry.spec.value),
      countable = entry.spec.count ~= nil,
      examples = entry.spec.examples,
    }
  end,
})

api.define = declarator("define", "functions", {
  declare = function(entry, path, spec, need)
    need(type(spec.impl) == "function", "impl must be a function")
    local where = placed(path, need, true)
    entry.where = where
    local tbl, name = table_of(where, need), where.name

    -- Where the function was put, so strict mode swaps the wrapper in without
    -- working the path out a second time. rawset: some module tables guard
    -- __newindex, and a declaration is not a caller poking at the module.
    entry.expose = function(fn)
      rawset(tbl, name, fn)
    end

    -- The raw implementation is the public function. No wrapper, no overhead.
    entry.expose(bound(spec.impl, path, spec.params))
    return spec.impl
  end,

  describe = function(entry)
    return {
      path = entry.path,
      description = entry.spec.description,
      params = as_doc_list(entry.spec.params),
      returns = as_doc_list(entry.spec.returns),
      examples = entry.spec.examples,
    }
  end,
})

-- Declares a handle type: which C members are public, under what name, plus the
-- methods and computed members that complete the type.
--
-- Nothing is reachable from a handle until it is named here. The C table says
-- how to reach a member; this says whether it is part of the API and what it is
-- called. A member C can reach but nobody declares simply does not exist.
-- A derived class carries the one it extends as its own metatable's __index.
-- That is what the checker walks to let an ItemQuery satisfy a Query, and what
-- makes an inherited method reachable. Lua looks a metamethod up raw, so
-- `__index` on the class is not what reaches one: a derived class carries its
-- own copy of the operators it inherits.
local function inherit(class, path, extends, need)
  local parent = at(extends)
  local parent_class = parent ~= nil and parent.class or nil
  need(parent_class ~= nil, "%s extends an undeclared type", path)
  setmetatable(class, { __index = parent_class })
  for key, value in pairs(parent_class) do
    if key:sub(1, 2) == "__" and key ~= "__index" then
      rawset(class, key, value)
    end
  end
  return parent
end

-- A field of a type written in Lua is a pair of accessors, since the value has
-- no struct behind it to address. A field is writable when it declares a `set`,
-- and read-only otherwise.
--
-- A field with no accessors is an entry the value carries itself, which is what
-- a plain table of numbers is: the class says what the keys are called and what
-- they hold, and reading one reaches the entry.
local function accessors(fields, need)
  local getters, setters = {}, {}
  for name, field in pairs(fields or {}) do
    if field.get == nil then
      need(field.set == nil, "field '%s' has a set but no get", name)
    else
      need(
        field.set == nil or type(field.set) == "function",
        "field '%s' has a set that is not a function",
        name
      )
      getters[name], setters[name] = field.get, field.set
    end
  end
  return getters, setters
end

-- Whether the docs say a field can be written. A handle's field addresses a
-- struct member and a Lua class's is a pair of accessors, so either may be
-- read-only. A record a call hands back is a plain table the caller owns, and
-- has nothing to say either way.
local function field_writable(spec, field)
  if spec.backing ~= nil then
    return field.writable ~= false
  end
  if field.get ~= nil then
    return field.set ~= nil
  end
  return nil
end

-- Declaring an accessor is what puts the type's members under the registry's
-- control: reading one the declaration does not name comes back nil, and
-- writing one raises, so a value cannot quietly grow members the docs never
-- see.
local function reach_fields(class, path, getters, setters)
  class.__index = function(self, key)
    local getter = getters[key]
    if getter ~= nil then
      return getter(self)
    end
    -- The metamethods sit on the class alongside the methods, and are the
    -- registry's business rather than a member of the type.
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

-- A type written in Lua names no C struct. The declaration owns its class table
-- and hands it back: the module gives a value that class as its metatable, and
-- every method a script can call is one declared here. `extends` names a type
-- whose methods this one inherits, for a domain that adds its own to a shared
-- base.
local function declare_lua_class(entry, path, spec, need)
  -- The docs read an extension off the declaration either way, so a class that
  -- named one would document a member nothing installs.
  need(
    next(spec.extensions or {}) == nil,
    "an extension is computed on a handle; a type written in Lua has none"
  )

  local class = {}
  class.__index = class

  local getters, setters = accessors(spec.fields, need)
  if spec.extends ~= nil then
    local parent = inherit(class, path, spec.extends, need)
    -- A field the parent declares reads on this type too. Its accessors sit in
    -- the parent's __index closure rather than in the class table, so walking
    -- the metatable chain does not reach one and they are taken over here.
    for name, getter in pairs(parent.getters) do
      if getters[name] == nil then
        getters[name], setters[name] = getter, parent.setters[name]
      end
    end
  end

  for name, operator in pairs(spec.operators or {}) do
    need(
      type(operator.impl) == "function",
      "operator '%s' needs an impl",
      name
    )
    rawset(class, "__" .. name, operator.impl)
  end

  entry.getters, entry.setters = getters, setters
  if next(getters) ~= nil then
    reach_fields(class, path, getters, setters)
  end

  entry.class = class
  -- A value the registry hands out is checked by identity: it really came from
  -- us, and an ItemQuery counts as a Query.
  entry.check = check.by_identity(class)
  -- A method of a type written in Lua is the Lua function it declares, and
  -- nothing stands behind it where the declaration names none.
  for name, method in pairs(spec.methods or {}) do
    need(type(method.impl) == "function", "%s.%s needs an impl", path, name)
  end
  bind_type_methods(entry)
  return class
end

-- A record is a plain table with no class on it: one a script writes out as an
-- options table, or one a call hands back for the caller to own. There is no
-- identity to check it by, so it is checked by what it holds - every key it
-- names, no key it does not, and each of the type it was given.
--
-- Which of the two a type is, the declaration says. Reading it off the shape -
-- a type with keys and no methods being a record - is a guess that goes wrong
-- the moment a record grows a method, or a type the registry hands out has
-- nothing but keys, as math.Box has.
local function declare_record(entry, spec, need)
  need(
    spec.extends == nil and next(spec.methods or {}) == nil,
    "a record is a plain table, so it has no methods and no parent"
  )
  need(
    spec.backing == nil
      and next(spec.operators or {}) == nil
      and next(spec.extensions or {}) == nil,
    "a record is a plain table, so it has no C struct, no operators and no extensions"
  )
  need(
    type(spec.fields) == "table" and next(spec.fields) ~= nil,
    "a record is checked by the keys it names, so it has to name some"
  )
  for name, field in pairs(spec.fields) do
    need(
      field.get == nil and field.set == nil,
      "field '%s' has an accessor, which a record has nowhere to keep",
      name
    )
  end
  entry.check = check.by_keys(spec.fields)
end

-- A handle stands for something the engine owns and can outlive it. C says how
-- to reach a member; the declaration says whether it is part of the API and
-- what it is called, and declaring the type is what makes its name checkable
-- in a params list.
local function declare_handle(entry, spec, need)
  local backing = spec.backing
  need(type(backing) == "string", "backing must be a C type name")
  -- A handle carries its C type name as its metatable, and has no class table
  -- for an operator to go on.
  need(
    next(spec.operators or {}) == nil,
    "an operator goes on a class table; a handle has none"
  )
  entry.check = check.by_metatable(backing)

  for name, field in pairs(spec.fields or {}) do
    struct.expose_field(
      backing,
      name,
      field.from or name,
      field.writable ~= false
    )
  end

  for name, computed in pairs(spec.extensions or {}) do
    need(
      type(computed.impl) == "function",
      "extension '%s' needs an impl",
      name
    )
    struct.expose_computed(backing, name, computed.impl)
  end

  bind_type_methods(entry)
end

api.type = declarator("type", "types", {
  declare = function(entry, path, spec, need)
    -- A type, a number and a unit each belong to the module that hands one out,
    -- and the name a params list writes is that path.
    entry.where = placed(path, need)
    if spec.record then
      return declare_record(entry, spec, need)
    end
    if spec.backing == nil then
      return declare_lua_class(entry, path, spec, need)
    end
    return declare_handle(entry, spec, need)
  end,

  describe = function(entry)
    local spec = entry.spec
    return {
      path = entry.path,
      description = spec.description,
      handle = spec.backing ~= nil,
      operators = members(spec.operators, function(operator)
        return { description = operator.description }
      end),
      fields = members(spec.fields, function(field)
        local doc = as_doc(field)
        return {
          type = doc.type,
          list = doc.list,
          writable = field_writable(spec, field),
          description = doc.description,
          -- A key of a table a script writes out may be left out, and stand in
          -- what the declaration says it stands in.
          optional = doc.optional,
          default = doc.default,
        }
      end),
      methods = members(spec.methods, function(method)
        return {
          description = method.description,
          params = as_doc_list(method.params),
          returns = as_doc_list(method.returns),
          examples = method.examples,
        }
      end),
      extensions = members(spec.extensions, function(computed)
        return {
          type = computed.type,
          description = computed.description,
        }
      end),
    }
  end,
})

-- The class of a type written in Lua, for a module that hands one of its values
-- out but is not where it is declared: trx.items hands back a math.Box, and a
-- box carries that class. Everything else names a type by its path.
--
-- A record has no class - a plain table is what it is - so asking for one says
-- so rather than handing back something inert to put on a value.
local class_needs = complain("api.class")

function api.class(path)
  local entry = at(path)
  local class = entry ~= nil and entry.class or nil
  class_needs(class ~= nil, "'%s' is not a declared type with a class", path)
  return class
end

-- Declares an enum: what the constants of a C enum are called in Lua, and what
-- they mean. As with api.type, C is the mechanism and this is the contract -
-- the names and values are reflected out of ENUM_MAP (see trx/game/enum.c), so
-- a number is never written twice and the two cannot drift.
--
-- Unlike a struct, an enum is small and wholly public: there is nothing to
-- hide, so exposure is not opt-in. Every constant must be documented, and
-- documenting one that does not exist is an error. The name a constant goes by
-- in Lua. `strip` takes a prefix off the reflected name: the C spelling is what
-- the data files are keyed by and cannot move, but
-- trx.lara.ExtraMesh.EXTRA_MESH_OAR only says EXTRA_MESH twice.
function api.enum_name(spec, reflected_name)
  local strip = spec.strip
  if strip ~= nil and reflected_name:sub(1, #strip) == strip then
    return reflected_name:sub(#strip + 1)
  end
  return reflected_name
end

api.enum = declarator("enum", "enums", {
  declare = function(entry, path, spec, need)
    need(type(spec.backing) == "string", "backing must be a C enum name")
    local where = placed(path, need)
    entry.where = where

    -- `bulk` is for a catalog-sized enum - every object in the game, say. It is
    -- described as a whole, and its constants carry no description apiece.
    local bulk = spec.bulk == true
    need(bulk or type(spec.values) == "table", "values must be a table")

    local public = {}
    local reflected = {}
    for _, constant in ipairs(enum.values(spec.backing)) do
      local value_name = api.enum_name(spec, constant.name)
      -- Stripping a prefix can collide two C constants onto one Lua name, and the
      -- second would quietly take the first one's place.
      need(
        public[value_name] == nil,
        "%s.%s is the name of two constants of %s",
        path,
        value_name,
        spec.backing
      )
      if not bulk then
        need(
          spec.values[value_name] ~= nil,
          "%s.%s is not documented",
          path,
          value_name
        )
      end
      public[value_name] = constant.value
      reflected[value_name] = true
    end

    for value_name in pairs(spec.values or {}) do
      need(
        reflected[value_name],
        "%s.%s is not a constant of %s",
        path,
        value_name,
        spec.backing
      )
    end

    -- How many constants the enum has, which the declaration did not say and
    -- the dump reads back off the entry.
    entry.count = 0
    for _ in pairs(public) do
      entry.count = entry.count + 1
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
        return public[key] or public[key:upper()]
      end,
      __newindex = function(_, key)
        error(
          ("trx.%s.%s: an enum cannot be written to"):format(
            path,
            tostring(key)
          ),
          2
        )
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

    rawset(module_table(where.module), where.name, face)

    -- What the engine passes is a number. Which numbers have names is the
    -- catalog's business - a bulk enum runs to hundreds, and carries ids no
    -- constant names - so a declaration typed by an enum checks for the number.
    entry.check = check.primitive("integer")

    return face
  end,

  describe = function(entry)
    local path, spec = entry.path, entry.spec
    local out = {
      path = path,
      description = spec.description,
      examples = spec.examples,
      bulk = spec.bulk == true,
      count = entry.count,
      values = {},
    }
    -- A bulk enum reads as names only, and no values: the ids are TRX's own, and
    -- a script refers to them by name.
    if out.bulk then
      out.names = {}
      for _, constant in ipairs(enum.values(spec.backing)) do
        table.insert(out.names, api.enum_name(spec, constant.name))
      end
      table.sort(out.names)
    else
      for _, constant in ipairs(enum.values(spec.backing)) do
        local value_name = api.enum_name(spec, constant.name)
        table.insert(out.values, {
          name = value_name,
          value = constant.value,
          description = spec.values[value_name],
        })
      end
      -- Numeric order: the order the constants are meant to be read in, and
      -- stable across dumps, which the reflected order is not.
      table.sort(out.values, function(a, b)
        return a.value < b.value
      end)
    end
    return out
  end,
})

-- Declares a lone constant sitting on the module table - an angle unit is a
-- macro, not a C enum, so api.enum has nothing to reflect. The value still
-- comes from C, so naming one C does not export fails here rather than quietly
-- being nil.
api.const = declarator("const", "constants", {
  declare = function(entry, path, spec, need)
    need(spec.value ~= nil, "%s has no value; is it exported from C?", path)
    local where = placed(path, need)
    entry.where = where

    rawset(module_table(where.module), where.name, spec.value)
    return spec.value
  end,

  describe = function(entry)
    local doc = as_doc(entry.spec)
    return {
      path = entry.path,
      value = doc.value,
      type = doc.type,
      description = doc.description,
    }
  end,
})

-- Declares a computed member on a module table, or on a namespace inside it.
-- It is not a function and not a stored field: reading it calls into C, and
-- writing it calls a setter, or fails if there is none. The registry owns the
-- container's metatable - see install_meta - so a hand-rolled getter table
-- would sail past seal().
api.property = declarator("property", "properties", {
  declare = function(entry, path, spec, need)
    need(type(spec.get) == "function", "get must be a function")
    need(
      spec.set == nil or type(spec.set) == "function",
      "set must be a function"
    )
    local where = placed(path, need, true)
    entry.where = where
    -- A group of properties is only the table its members hang off, so it need
    -- not be declared: trx.rules.exposure.max asks for trx.rules.exposure and
    -- gets it. api.namespace is for a group that has something of its own to
    -- say, which is why api.define still insists on one.
    if
      where.namespace ~= nil
      and rawget(module_table(where.module), where.namespace) == nil
    then
      api.namespace(where.owner, { implicit = true })
    end

    -- A property has an entry of its own, and the table it hangs off gathers the
    -- ones declared on it: what its __index dispatches on is the owner's, not
    -- each property's.
    local owner_entry = open(where.owner)
    owner_entry.table = table_of(where, need)
    owner_entry.properties = owner_entry.properties or {}
    owner_entry.properties[where.name] = entry

    install_meta(owner_entry)
    return spec
  end,

  describe = function(entry)
    local doc = as_doc(entry.spec)
    return {
      path = entry.path,
      type = doc.type,
      list = doc.list,
      description = doc.description,
      writable = entry.spec.set ~= nil,
    }
  end,
})

-- Declares a number: what it counts, and where it counts from. A room number
-- is one thing however many declarations hold one, so it is written here and
-- they name it. Doc-only in the sense that no member of it reaches a script -
-- what a script gets is the number itself - but it is a type like any other,
-- and a declaration that holds one says so.
api.number = declarator("number", "numbers", {
  declare = function(entry, path, spec, need)
    entry.where = placed(path, need)
    need(type(spec.description) == "string", "description must be a string")
    need(
      spec.base == nil or spec.base == 0 or spec.base == 1,
      "base counts from 0 or from 1"
    )
    entry.check = check.primitive("integer")
  end,

  describe = function(entry)
    return {
      path = entry.path,
      description = entry.spec.description,
      base = entry.spec.base,
    }
  end,
})

-- Declares a unit: what a value is measured in. An angle is measured the same
-- way wherever one turns up, so it is written here and the declarations that
-- hold one name it. Doc-only as a number is, and a type like any other.
--
-- `type` is what a value of it is: whole where the engine counts in steps of
-- one, as it does for an angle and a distance, and a real number where it does
-- not, as for a time in seconds.
api.unit = declarator("unit", "units", {
  declare = function(entry, path, spec, need)
    entry.where = placed(path, need)
    need(type(spec.description) == "string", "description must be a string")
    local measured = spec.type or "integer"
    need(
      measured == "integer" or measured == "number",
      "a unit measures an integer or a number"
    )
    need(
      spec.spellings == nil or type(spec.spellings) == "table",
      "spellings must list the words that mean this unit"
    )
    entry.check = check.primitive(measured)
  end,

  describe = function(entry)
    local spec = entry.spec
    return {
      path = entry.path,
      description = spec.description,
      -- What a value of it is. Written out even where it is the default, so
      -- the docs need not know what that default is.
      type = spec.type or "integer",
      spellings = spec.spellings,
    }
  end,
})

-- Turning checking on, and closing the registry.

-- Rebinds every registered function to (or away from) its checking wrapper.
function api.strict(enabled)
  strict_enabled = enabled and true or false
  for _, entry in ipairs(each(api.define)) do
    entry.expose(bound(entry.spec.impl, entry.path, entry.spec.params))
  end
  for _, entry in ipairs(each(api.namespace)) do
    if entry.call ~= nil then
      entry.call = bound(entry.spec.call, entry.path, entry.spec.params)
    end
  end
  for _, entry in ipairs(each(api.type)) do
    bind_type_methods(entry)
  end
end

function api.is_strict()
  return strict_enabled
end

-- Anything a script can reach on a module or a namespace table has to be
-- declared, or the reference cannot describe it. Neither a metatable getter
-- nor a struct field ever shows up in pairs(), so this is what catches one.
local function audit_reachable()
  -- What a declaration puts on a table a script can index, named by the
  -- declarator that makes one. A type, a number and a unit put nothing there,
  -- and a property is served by the metatable rather than held in the table, so
  -- neither shows up below. The namespace table itself is a member of its
  -- module.
  local ON_A_TABLE = {}
  for _, maker in ipairs({ api.define, api.enum, api.const, api.namespace }) do
    ON_A_TABLE[KIND_OF[maker]] = true
  end

  -- table path ("items", or "console.log") -> set of declared member names.
  local declared = {}
  for _, path in ipairs(declarations.order) do
    local entry = declarations.by_path[path]
    if ON_A_TABLE[entry.kind] then
      local owner = entry.where.owner
      declared[owner] = declared[owner] or {}
      declared[owner][entry.where.name] = true
    elseif made_by(entry, api.container) and entry.member ~= nil then
      -- The table a collection is read through is a member of its module.
      declared[entry.module] = declared[entry.module] or {}
      declared[entry.module][entry.member] = true
    end
  end

  local undeclared = {}
  local function audit(table_path, tbl)
    -- next, not pairs: a container overrides __pairs to walk its collection, so
    -- the module's own members are only reachable through a raw iteration.
    for name in next, tbl or {} do
      if not (declared[table_path] or {})[name] then
        undeclared[#undeclared + 1] = "trx." .. table_path .. "." .. name
      end
    end
  end

  -- A module is read live rather than off its entry: sealing has just replaced
  -- trx.api with what survives it, and the audit is of the surface a script is
  -- left with. A namespace hides its members one level down, where a module's
  -- own audit cannot see them, so each is audited as the table it is. So is the
  -- table a collection is read through, which is a member of its module and
  -- holds what was declared inside it.
  for _, entry in ipairs(each(api.module)) do
    audit(entry.path, trx[entry.path])
  end
  for _, entry in ipairs(each(api.namespace)) do
    audit(entry.path, entry.table)
  end
  for _, entry in ipairs(each(api.container)) do
    if entry.member ~= nil then
      local owner = at(entry.module .. "." .. entry.member)
      audit(owner.path, owner.table)
    end
  end

  if #undeclared > 0 then
    table.sort(undeclared)
    error(
      table.concat(undeclared, ", ")
        .. ": reachable from scripts but not declared, so the reference "
        .. "cannot describe it. Declare it with api.define, api.enum, "
        .. "api.const, api.property, api.namespace or api.container."
    )
  end
end

-- A type nothing can check prints a name that means nothing, and waves every
-- value through while strict mode reports a clean run over it. `partial` is for
-- a test that stands up part of the surface, where a name the rest of it
-- declares is expected to be missing.
--
-- What is audited is what checking will call: the arguments of a function, of a
-- callable namespace and of a method, the keys of a table one of those takes,
-- and a property written through. A name in a doc-only position - what a call
-- hands back, what a constant is measured in - is resolved by the docs tool,
-- which walks the dump for exactly that.
local function audit_types(partial)
  local bad = {}
  local function report(fmt, ...)
    bad[#bad + 1] = fmt:format(...)
  end

  -- The predicate a declaration is checked by, or nil where nothing can check
  -- it, which is itself worth reporting.
  local function checked_by(where, what, spec)
    local predicate = check.of(spec)
    if predicate == nil and not partial then
      report(
        "%s: %s has an unknown type '%s'",
        where,
        what,
        check.label_of(spec)
      )
    end
    return predicate
  end

  -- The keys of a table a script writes out, which are checked as its own
  -- arguments are and can name a type in the same way.
  local function audit_keys(where, fields)
    for key, field in pairs(fields or {}) do
      local name = field.name or key
      local predicate = checked_by(where, ("the key '%s'"):format(name), field)
      if
        predicate ~= nil
        and field.default ~= nil
        and not predicate(field.default)
      then
        report(
          "%s: the default for '%s' is not %s",
          where,
          name,
          check.label_of(field)
        )
      end
    end
  end

  local function audit_params(where, params)
    local optional_seen = false
    for _, p in ipairs(params or {}) do
      if p.name ~= "..." then
        local predicate = checked_by(where, ("'%s'"):format(p.name), p)
        audit_keys(where .. "." .. p.name, p.fields)
        if
          predicate ~= nil
          and p.default ~= nil
          and not predicate(p.default)
        then
          report(
            "%s: the default for '%s' is not %s",
            where,
            p.name,
            check.label_of(p)
          )
        end
        -- Nothing can reach it without passing the optional one too.
        if p.optional then
          optional_seen = true
        elseif optional_seen then
          report(
            "%s: '%s' is required but follows an optional parameter",
            where,
            p.name
          )
        end
      end
    end
  end

  -- The two that take arguments a script writes.
  for _, maker in ipairs({ api.define, api.namespace }) do
    for _, entry in ipairs(each(maker)) do
      audit_params(entry.path, entry.spec.params)
    end
  end
  -- Strict mode checks a method's arguments too, and make_checked needs a
  -- checker for each. Without this, a type nobody can check would only surface
  -- the day a builder turned strict mode on.
  for _, entry in ipairs(each(api.type)) do
    for name, method in pairs(entry.spec.methods or {}) do
      audit_params(entry.path .. "." .. name, method.params)
    end
  end
  for _, entry in ipairs(each(api.property)) do
    checked_by(entry.path, "the property", entry.spec)
  end
  -- Strict mode holds a script to what a collection is indexed with, so the key
  -- has to answer for the same reason a parameter does.
  for _, entry in ipairs(each(api.container)) do
    checked_by(entry.path, "the key", entry.spec.key)
  end
  -- A record is checked by the keys it names wherever one is passed in, so the
  -- names have to answer for the same reason a parameter's does.
  for _, entry in ipairs(each(api.type)) do
    if entry.spec.record then
      audit_keys(entry.path, entry.spec.fields)
    end
  end

  if #bad > 0 then
    table.sort(bad)
    error(table.concat(bad, "\n"))
  end
end

-- Called once, from C, after the trx.* modules have loaded. Declarations are
-- the engine's to make; a level script re-opening the surface would defeat the
-- point of declaring it.
function api.seal(spec)
  -- The declaring half has done its job, and every one of those functions
  -- raises from here on. It goes the way trxc goes at this same moment: what a
  -- script cannot successfully call, it should not be able to reach. Done
  -- before the audits, so they see the surface a script is left with. C keeps
  -- the two entrypoints it still needs - see lua/capi/api.c.
  trx.api = {
    strict = api.strict,
    is_strict = api.is_strict,
  }
  -- The checking layer goes the same way. Nothing declares it, so the audit
  -- below never sees it, and reads_from is a way into what strict mode holds a
  -- script to.
  trx.check = nil

  audit_reachable()
  audit_types(spec ~= nil and spec.partial == true)

  sealed = true
end

-- The whole surface as plain data, and as JSON.

function api.describe()
  local out = {}
  for _, kind in ipairs(KINDS) do
    local list = {}
    for i, entry in ipairs(entries_of(kind)) do
      local described = kind.describe(entry)
      -- Read off the spec rather than from each kind's describe: the flag means
      -- the same thing wherever it is written, so no kind has to remember it.
      described.deprecated = entry.spec.deprecated
      list[i] = described
    end
    kind.sort(list)
    out[kind.key] = list
  end

  -- Every kind of entry carries prose under the same key, so the indentation
  -- comes off in one pass rather than at each place one is collected. An
  -- example is left alone: it is code, and its own indentation is the point.
  local function dedent_prose(node)
    if type(node) ~= "table" then
      return
    end
    for key, value in pairs(node) do
      if key == "description" and type(value) == "string" then
        node[key] = trx.strings.dedent(value)
      else
        dedent_prose(value)
      end
    end
  end
  dedent_prose(out)

  return out
end

-- The complete public API surface. One registry, one source: what is not
-- declared here is not reachable from a script, so the dump cannot omit
-- anything that exists.
function api.to_json()
  return trx.json.encode(api.describe())
end

-- The registry declares itself, last of all. What is left of it once seal() has
-- run is what a script can use: strict mode, and the question of whether it is
-- on. The rest declared the surface and is gone by the time any script runs.
api.module("api", {
  order = 35,
  title = "API registry",
  description = "Argument checking for the whole of `trx`.",
})

api.define("api.strict", {
  description = [[
    Turns argument checking on or off for every function in `trx`, and for the
    methods on its handles. Off by default: checking costs a couple of hundred
    nanoseconds a call, which a per-frame handler notices. Turn it on while
    writing a level, and leave it off in play.
  ]],
  params = {
    {
      name = "enabled",
      type = "boolean",
      description = "Whether to check.",
    },
  },
  examples = { [[trx.api.strict(true)]] },
  impl = api.strict,
})

api.define("api.is_strict", {
  description = "Whether argument checking is on.",
  returns = {
    type = "boolean",
    description = "False as the game starts, and true once something turns it on.",
  },
  impl = api.is_strict,
})

-- The registry, handed to scripts and to C, declaring itself last of all.

trx.api = api

-- api.type reports members nobody exposed, so the logger must exist by the time
-- any module declares one, and modules load in source-list order, which puts
-- log after items. Force it here rather than depend on that ordering.
--
-- It has to come after `trx.api` is set, not before: log.lua declares itself
-- through the registry, so requiring it any earlier would hand it a half-built
-- api table.
require("trx.log")

-- Sealing and dumping the surface stay C's to call, and the dump runs after the
-- seal has taken them off trx.api. Hand them over while they are still here.
capi.set_entrypoint("seal", api.seal)
capi.set_entrypoint("to_json", api.to_json)
