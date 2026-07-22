local api = trx.api

api.module("query", {
  order = 16,
  title = "Query",
  description = [[
A composable filter over a domain of things - the objects a level is built
from, or the items alive in it. `trx.objects.query` and `trx.items.query` are
each the identity query, matching everything; you narrow one down and then read
the result.

A query is immutable. Every method returns a fresh query, so a base can be kept
and branched from without one narrowing leaking into another.

Three ways to narrow, which mix freely:

- Chain methods read left to right and combine with AND:
  `q:spawnable():by_name("wolf")`. Each domain adds its own - see
  `trx.objects.query` and `trx.items.query` for the ones it has.
- The operators `&`, `|` and `~` are AND, OR and NOT over whole queries, for
  what a chain cannot say: `q:pickup() | q:inventory_item()`, `~q:animation()`.
  Both sides of `&`/`|` must come from the same domain.
- `by_name(name)` ranks rather than filters: it matches the way a player types
  a name, forgivingly, and orders what survives the rest of the query best
  first. Some of a domain's filters are also searchable groups - `pickup` is
  one - so their name matches every member. Only a domain that has names offers
  `by_name`, `names` and `best`.

Read a query with a terminal:

- `ids()` - the matching ids, a list. For objects these are object ids; for
  items, their numbers.
- `matches()` - the matching handles: `trx.objects.Object`s or
  `trx.items.Item`s.
- `first()` - the first matching handle, or `nil`.
- `count()` - how many match.
- `names()` - every name the matches answer to, group names included, for
  offering completions.
- `best()` - the ids tied for the best `by_name` score: one for a name only one
  thing answers to, the whole group for a group name.
]],
})

-- A domain is what a query runs against. It supplies:
--   enumerate() - every candidate as a { id, handle } pair, each id once.
--   id_of(id, handle) - what ids() yields for a candidate.
--   filters - named narrowings. Each is a function(...) returning a
--     predicate(id, handle), or a table { test = <that function>, searchable =
--     true } for a nullary filter whose own name a by_name also matches.
--   names_of(handle) - optional. The names a thing answers to, best weight. Its
--     presence is what gives a domain by_name, names and best.
--   default_names_of(handle) - optional. A fallback set tried when nothing in
--     names_of matched, for before a language file is loaded.

-- A fresh query over the same domain, carrying this one's metatable so the
-- domain's filters ride along.
local function derive(self, pred, name)
  return setmetatable(
    { _domain = self._domain, _pred = pred, _name = name },
    getmetatable(self)
  )
end

local function same_domain(a, b)
  assert(
    a._domain == b._domain,
    "cannot combine queries over different domains"
  )
end

local function band(a, b)
  same_domain(a, b)
  return derive(a, function(id, h)
    return a._pred(id, h) and b._pred(id, h)
  end, a._name or b._name)
end

local function bor(a, b)
  same_domain(a, b)
  return derive(a, function(id, h)
    return a._pred(id, h) or b._pred(id, h)
  end, a._name or b._name)
end

local function bnot(a)
  return derive(a, function(id, h)
    return not a._pred(id, h)
  end, a._name)
end

local function build(domain)
  local index = {}
  local meta = {
    __index = index,
    __band = band,
    __bor = bor,
    __bnot = bnot,
  }

  -- Each filter becomes a chain method that ANDs its predicate onto the query.
  -- A searchable one also joins the name candidates below, under its own key.
  local searchable = {}
  for name, spec in pairs(domain.filters) do
    local make = type(spec) == "function" and spec or spec.test
    index[name] = function(self, ...)
      local pred = make(...)
      return derive(self, function(id, h)
        return self._pred(id, h) and pred(id, h)
      end, self._name)
    end
    if type(spec) == "table" and spec.searchable then
      searchable[#searchable + 1] = { key = name, pred = make() }
    end
  end

  -- The pairs that pass the predicate, in enumeration order, before ranking.
  local function kept_pairs(self)
    local kept = {}
    for _, pair in ipairs(domain.enumerate()) do
      if self._pred(pair[1], pair[2]) then
        kept[#kept + 1] = pair
      end
    end
    return kept
  end

  -- rank stays nil for a domain without names, and the name terminals go
  -- undeclared, so a query that has no by_name has no names or best either.
  local rank

  if domain.names_of ~= nil then
    -- The names a lookup weighs: everything a thing answers to at full weight,
    -- and the name of every searchable group it belongs to at a weight a real
    -- name beats.
    local function candidates(kept, names_getter)
      local out = {}
      for _, pair in ipairs(kept) do
        for _, name in ipairs(names_getter(pair[2])) do
          out[#out + 1] = { key = name, value = pair, weight = 2 }
        end
        for _, group in ipairs(searchable) do
          if group.pred(pair[1], pair[2]) then
            out[#out + 1] = { key = group.key, value = pair, weight = 1 }
          end
        end
      end
      return out
    end

    -- The player's language is tried first; nothing there matching falls back
    -- on the names the engine was built with, present before any language file.
    rank = function(name, kept)
      local matches =
        trx.strings.fuzzy_match(name, candidates(kept, domain.names_of))
      if #matches == 0 and domain.default_names_of ~= nil then
        matches = trx.strings.fuzzy_match(
          name,
          candidates(kept, domain.default_names_of)
        )
      end
      -- One thing answers to several names, so it can match more than once. It
      -- comes back the once, at its best rank.
      local out, seen = {}, {}
      for _, match in ipairs(matches) do
        if not seen[match.value] then
          seen[match.value] = true
          out[#out + 1] = { pair = match.value, score = match.score }
        end
      end
      return out
    end

    index.by_name = function(self, name)
      return derive(self, self._pred, name)
    end

    -- The names to complete against: what each match answers to, plus a
    -- searchable group's name once any match belongs to it. Localized names,
    -- falling back on the built-in ones before a language is loaded.
    index.names = function(self)
      local kept = kept_pairs(self)
      local order, seen = {}, {}
      local function add(name)
        if not seen[name] then
          seen[name] = true
          order[#order + 1] = name
        end
      end
      for _, pair in ipairs(kept) do
        local names = domain.names_of(pair[2])
        if #names == 0 and domain.default_names_of ~= nil then
          names = domain.default_names_of(pair[2])
        end
        for _, name in ipairs(names) do
          add(name)
        end
      end
      for _, group in ipairs(searchable) do
        for _, pair in ipairs(kept) do
          if group.pred(pair[1], pair[2]) then
            add(group.key)
            break
          end
        end
      end
      return order
    end

    -- The strongest matches: those tied for the top score. A name only one
    -- thing answers to yields one; a group name yields the whole group.
    index.best = function(self)
      if self._name == nil then
        return self:ids()
      end
      local ranked = rank(self._name, kept_pairs(self))
      local out = {}
      local top = ranked[1] ~= nil and ranked[1].score or nil
      for _, entry in ipairs(ranked) do
        if entry.score == top then
          out[#out + 1] = domain.id_of(entry.pair[1], entry.pair[2])
        end
      end
      return out
    end
  end

  -- The matches a terminal reads: ranked by name when one is set, otherwise the
  -- kept pairs in enumeration order.
  local function resolved(self)
    local kept = kept_pairs(self)
    if self._name == nil or rank == nil then
      return kept
    end
    local out = {}
    for _, entry in ipairs(rank(self._name, kept)) do
      out[#out + 1] = entry.pair
    end
    return out
  end

  index.ids = function(self)
    local out = {}
    for _, pair in ipairs(resolved(self)) do
      out[#out + 1] = domain.id_of(pair[1], pair[2])
    end
    return out
  end

  index.matches = function(self)
    local out = {}
    for _, pair in ipairs(resolved(self)) do
      out[#out + 1] = pair[2]
    end
    return out
  end

  index.first = function(self)
    local pair = resolved(self)[1]
    return pair ~= nil and pair[2] or nil
  end

  index.count = function(self)
    return #resolved(self)
  end

  return setmetatable({
    _domain = domain,
    _pred = function()
      return true
    end,
    _name = nil,
  }, meta)
end

api.define("query.new", {
  description = "Builds the identity query for a domain. `trx.objects` and `trx.items` call this to "
    .. "make the query a script reaches through `trx.objects.query` and `trx.items.query`.",
  params = {
    {
      name = "domain",
      type = "table",
      description = "What the query runs against: `enumerate`, `id_of`, `filters`, and an optional "
        .. "`names_of`/`default_names_of` name layer. See the module source.",
    },
  },
  returns = {
    type = "table",
    description = "The identity query, matching everything until narrowed.",
  },
  impl = build,
})
