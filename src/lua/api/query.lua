local api = trx.api

api.module("query", {
  order = 9,
  title = "Query",
  description = [[
A composable filter over a domain of things - the objects a level is built
from, or the items alive in it. `trx.objects.query` and `trx.items.query` are
each the identity query, matching everything; narrow one down, then read the
result.

A query is immutable. Every method returns a fresh query, so a base can be kept
and branched from without one narrowing leaking into another.

Each domain adds narrowings of its own on top of the ones below - see
`trx.items.ItemQuery` and `trx.objects.ObjectQuery` - and chained methods read
left to right, combining with AND: `q:spawnable():by_name("wolf")`.
]],
})

-- A domain is what a query runs against. It supplies:
--   enumerate() - every candidate as a { id, handle } pair, each id once.
--   id_of(id, handle) - what ids() yields for a candidate.
--   searchable - the filters whose own name a by_name also matches, as
--     { key, pred } pairs in the order a completer offers them.
--   names_of(handle) - optional. The names a thing answers to, outranked only
--     by a group spelled out in full. Its presence is what gives a domain
--     by_name, names and best.
--   default_names_of(handle) - optional. A fallback set tried when nothing in
--     names_of matched, so that a name the engine was built with reaches a
--     thing whatever language the player reads.

-- A fresh query over the same domain, carrying this one's class so the domain's
-- own narrowings ride along.
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

-- The pairs that pass the predicate, in enumeration order, before ranking.
local function kept_pairs(self)
  local kept = {}
  for _, pair in ipairs(self._domain.enumerate()) do
    if self._pred(pair[1], pair[2]) then
      kept[#kept + 1] = pair
    end
  end
  return kept
end

-- The names a lookup weighs: a group spelled out in full above everything,
-- since the player naming a group means the group and not whichever of its
-- members happens to answer to the same word; then everything a thing answers
-- to; then the groups it belongs to as a partial match.
local NAME_WEIGHT = 2
local GROUP_WEIGHT = 1
local SPELLED_GROUP_WEIGHT = 3

local function candidates(domain, kept, names_getter, spelled)
  local out = {}
  for _, pair in ipairs(kept) do
    for _, name in ipairs(names_getter(pair[2])) do
      out[#out + 1] = { key = name, value = pair, weight = NAME_WEIGHT }
    end
    for _, group in ipairs(domain.searchable) do
      if group.pred(pair[1], pair[2]) then
        out[#out + 1] = {
          key = group.key,
          value = pair,
          weight = group.key == spelled and SPELLED_GROUP_WEIGHT
            or GROUP_WEIGHT,
        }
      end
    end
  end
  return out
end

-- The player's language is tried first; nothing there matching falls back on
-- the names the engine was built with, so an English name still reaches a
-- thing on a translated install.
local function rank(domain, name, kept)
  local spelled = name:lower():match("^%s*(.-)%s*$")
  local matches = trx.strings.fuzzy_match(
    name,
    candidates(domain, kept, domain.names_of, spelled)
  )
  if #matches == 0 and domain.default_names_of ~= nil then
    matches = trx.strings.fuzzy_match(
      name,
      candidates(domain, kept, domain.default_names_of, spelled)
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

-- The matches a terminal reads: ranked by name when one is set, otherwise the
-- kept pairs in enumeration order.
local function resolved(self)
  local kept = kept_pairs(self)
  if self._name == nil then
    return kept
  end
  local out = {}
  for _, entry in ipairs(rank(self._domain, self._name, kept)) do
    out[#out + 1] = entry.pair
  end
  return out
end

local QUERY = { type = "query.Query", description = "The narrowed query." }

local Query = api.type("query.Query", {
  description = "A filter over a domain, read with one of the terminals below once it is narrow "
    .. "enough.",

  operators = {
    band = {
      description = "Both queries match. Their domains must agree.",
      impl = function(a, b)
        same_domain(a, b)
        return derive(a, function(id, h)
          return a._pred(id, h) and b._pred(id, h)
        end, a._name or b._name)
      end,
    },
    bor = {
      description = "Either query matches, for what a chain cannot say: "
        .. "`q:pickup() | q:inventory_item()`. Their domains must agree.",
      impl = function(a, b)
        same_domain(a, b)
        return derive(a, function(id, h)
          return a._pred(id, h) or b._pred(id, h)
        end, a._name or b._name)
      end,
    },
    bnot = {
      description = "Everything the query does not match: `~q:animation()`.",
      impl = function(a)
        return derive(a, function(id, h)
          return not a._pred(id, h)
        end, a._name)
      end,
    },
  },

  methods = {
    where = {
      description = "Narrows by a test of the caller's own, for what the domain does not name.",
      params = {
        {
          name = "predicate",
          type = "function",
          description = "The test each candidate is put through.",
          params = {
            {
              name = "id",
              type = "integer",
              description = "The candidate's id.",
            },
            {
              name = "handle",
              type = "any",
              description = "The candidate itself, a `trx.objects.Object` or a `trx.items.Item`.",
            },
          },
        },
      },
      returns = QUERY,
      examples = {
        [[local hurt = trx.items.query:where(function(id, item)
  return item.hit_points < 10
end)]],
      },
      impl = function(self, pred)
        return derive(self, function(id, h)
          return self._pred(id, h) and pred(id, h)
        end, self._name)
      end,
    },

    ids = {
      description = "The matching ids. For objects these are object ids; for items, their numbers.",
      returns = { type = "integer", list = true },
      impl = function(self)
        local out = {}
        for _, pair in ipairs(resolved(self)) do
          out[#out + 1] = self._domain.id_of(pair[1], pair[2])
        end
        return out
      end,
    },

    matches = {
      description = "The matching handles.",
      returns = { type = { "objects.Object", "items.Item" }, list = true },
      impl = function(self)
        local out = {}
        for _, pair in ipairs(resolved(self)) do
          out[#out + 1] = pair[2]
        end
        return out
      end,
    },

    first = {
      description = "The first matching handle.",
      returns = {
        type = "any",
        nullable = true,
        description = "The handle, or `nil`.",
      },
      impl = function(self)
        local pair = resolved(self)[1]
        return pair ~= nil and pair[2] or nil
      end,
    },

    count = {
      description = "How many candidates match.",
      returns = {
        type = "integer",
        description = "The count, without building the list.",
      },
      impl = function(self)
        return #resolved(self)
      end,
    },
  },
})

api.type("query.NamedQuery", {
  extends = "query.Query",
  description = "A query over a domain whose things answer to names, which adds the name layer to "
    .. "everything a `trx.query.Query` has. A domain without names offers none of it.",

  methods = {
    by_name = {
      description = [[Ranks rather than filters: matches the way a player types a name,
forgivingly, and orders what survives the rest of the query best first. Some of a domain's
narrowings are also searchable groups, so their own name matches every member. A group named
in full comes first, ahead of anything that answers to the same word.]],
      params = {
        { name = "name", type = "string", description = "What to look for." },
      },
      returns = QUERY,
      examples = { [[trx.objects.query:spawnable():by_name("wolf"):ids()]] },
      impl = function(self, name)
        return derive(self, self._pred, name)
      end,
    },

    names = {
      description = "Every name the matches answer to, for offering completions. The group names "
        .. "any match belongs to come first, because a completer offers the list in order and a "
        .. "group name that ties on score would otherwise sit behind a thing's own. Which groups "
        .. "answer follows from what the query kept, so one narrowed to what fights offers no "
        .. "`trx.objects.ObjectQuery:pickup`.",
      returns = { type = "string", list = true },
      impl = function(self)
        local domain = self._domain
        local kept = kept_pairs(self)
        local order, seen = {}, {}
        local function add(name)
          if not seen[name] then
            seen[name] = true
            order[#order + 1] = name
          end
        end
        for _, group in ipairs(domain.searchable) do
          for _, pair in ipairs(kept) do
            if group.pred(pair[1], pair[2]) then
              add(group.key)
              break
            end
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
        return order
      end,
    },

    best = {
      description = [[The ids tied for the best `trx.query.NamedQuery:by_name` score: one for a
name only one thing answers to, the whole group for a group named in full. Without a
`trx.query.NamedQuery:by_name`, every matching id.]],
      returns = { type = "integer", list = true },
      impl = function(self)
        if self._name == nil then
          return self:ids()
        end
        local domain = self._domain
        local ranked = rank(domain, self._name, kept_pairs(self))
        local out = {}
        local top = ranked[1] ~= nil and ranked[1].score or nil
        for _, entry in ipairs(ranked) do
          if entry.score == top then
            out[#out + 1] = domain.id_of(entry.pair[1], entry.pair[2])
          end
        end
        return out
      end,
    },
  },
})

-- A domain's own narrowing, as a method on its query type: the filter's
-- predicate ANDed onto the query, which is what every one of them does.
api.define("query.narrowing", {
  description = "Builds a narrowing method for a domain's query type out of a predicate factory. "
    .. "`trx.items` and `trx.objects` declare their own filters with it.",
  params = {
    {
      name = "make",
      type = "function",
      description = "Called with the method's own arguments, returning a `predicate(id, handle)`.",
    },
  },
  returns = {
    type = "function",
    description = "The method to declare as an `impl`. <!--noref: impl-->",
  },
  impl = function(make)
    return function(self, ...)
      local pred = make(...)
      return derive(self, function(id, h)
        return self._pred(id, h) and pred(id, h)
      end, self._name)
    end
  end,
})

api.define("query.new", {
  description = "Builds the identity query over a domain, as an instance of that domain's query "
    .. "type. `trx.objects` and `trx.items` call this to make the query a script reaches through "
    .. "`trx.objects.query` and `trx.items.query`.",
  params = {
    {
      name = "domain",
      type = "table",
      description = "What the query runs against.",
      fields = {
        {
          name = "enumerate",
          type = "function",
          description = "Every id the domain holds.",
        },
        {
          name = "id_of",
          type = "function",
          description = "The id of a thing the domain hands out.",
        },
        {
          name = "searchable",
          type = "function",
          description = "Whether an id is one a name may reach.",
        },
        {
          name = "names_of",
          type = "function",
          optional = true,
          description = "The names an id answers to, for a domain that has them.",
        },
        {
          name = "default_names_of",
          type = "function",
          optional = true,
          description = "The names the engine was built with. The query tries "
            .. "these when `names_of` finds no match. <!--noref: names_of-->",
        },
      },
    },
    {
      name = "class",
      type = "table",
      description = "The query type the domain's narrowings were declared on.",
    },
  },
  returns = {
    type = "query.Query",
    description = "The identity query, matching everything until narrowed.",
  },
  impl = function(domain, class)
    domain.searchable = domain.searchable or {}
    return setmetatable({
      _domain = domain,
      _pred = function()
        return true
      end,
      _name = nil,
    }, class or Query)
  end,
})
