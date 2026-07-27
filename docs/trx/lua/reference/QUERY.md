---
title: Query
order: 16
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/query.lua. Edit it there.
-->

## Query module

A composable filter over a domain of things - the objects a level is built
from, or the items alive in it. `trx.objects.query` and `trx.items.query` are
each the identity query, matching everything; you narrow one down and then read
the result.

A query is immutable. Every method returns a fresh query, so a base can be kept
and branched from without one narrowing leaking into another.

Four ways to narrow, which mix freely:

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
- `where(predicate)` narrows by a test of your own, called with the id and the
  handle, for what a domain does not name: `q:where(function(id, item) return
  item.timer > 0 end)`.

Read a query with a terminal:

- `ids()` - the matching ids, a list. For objects these are object ids; for
  items, their numbers.
- `matches()` - the matching handles: `trx.objects.Object`s or
  `trx.items.Item`s.
- `first()` - the first matching handle, or `nil`.
- `count()` - how many match.
- `names()` - every name the matches answer to, for offering completions. The
  group names any match belongs to come first.
- `best()` - the ids tied for the best `by_name` score: one for a name only one
  thing answers to, the whole group for a group name.


### Functions

- [lua]`trx.query.new(domain)`  
  Builds the identity query for a domain. `trx.objects` and `trx.items` call this to make the query a script reaches through `trx.objects.query` and `trx.items.query`.

  Parameters:
  - **`domain`** (table). What the query runs against: `enumerate`, `id_of`, `filters`, and an optional `names_of`/`default_names_of` name layer. See the module source.

  Returns: table. The identity query, matching everything until narrowed.
