---
title: Query
order: 9
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
each the identity query, matching everything; narrow one down, then read the
result.

A query is immutable. Every method returns a fresh query, so a base can be kept
and branched from without one narrowing leaking into another.

Each domain adds narrowings of its own on top of the ones below - see
`trx.items.ItemQuery` and `trx.objects.ObjectQuery` - and chained methods read
left to right, combining with AND: `q:spawnable():by_name("wolf")`.

### Structures

- [lua]`trx.query.Query`

    A filter over a domain, read with one of the terminals below once it is narrow enough.

    Operators:
    - **`query & query`**. Both queries match. Their domains must agree.
    - **`~query`**. Everything the query does not match: `~q:animation()`.
    - **`query | query`**. Either query matches, for what a chain cannot say: `q:pickup() | q:inventory_item()`. Their domains must agree.

    Methods:

    - [lua]`query:count()`  
      How many candidates match.

      Returns: integer.

    - [lua]`query:first()`  
      The first matching handle.

      Returns: any or `nil`. The handle, or `nil`.

    - [lua]`query:ids()`  
      The matching ids. For objects these are object ids; for items, their numbers.

      Returns: table. A list of integers.

    - [lua]`query:matches()`  
      The matching handles.

      Returns: table. A list of `trx.objects.Object`s or `trx.items.Item`s.

    - [lua]`query:where(predicate)`  
      Narrows by a test of the caller's own, for what the domain does not name.

      Parameters:
      - **`predicate`** (function).
        Called with:
        - **`id`** (integer). The candidate's id.
        - **`handle`** (any). The candidate itself, an `Object` or an `Item`.

      Returns: Query. The narrowed query.

      Example:
      ```lua
      local hurt = trx.items.query:where(function(id, item)
        return item.hit_points < 10
      end)
      ```

- [lua]`trx.query.NamedQuery`

    A query over a domain whose things answer to names, which adds the name layer to everything a `Query` has. A domain without names offers none of it.

    Methods:

    - [lua]`namedquery:best()`  
      The ids tied for the best `by_name` score: one for a name only one thing answers to, the whole group for a group name. Without a `by_name`, every matching id.

      Returns: table. A list of integers.

    - [lua]`namedquery:by_name(name)`  
      Ranks rather than filters: matches the way a player types a name, forgivingly, and orders what survives the rest of the query best first. Some of a domain's narrowings are also searchable groups, so their own name matches every member.

      Parameters:
      - **`name`** (string). What to look for.

      Returns: Query. The narrowed query.

      Example:
      ```lua
      trx.objects.query:spawnable():by_name("wolf"):ids()
      ```

    - [lua]`namedquery:names()`  
      Every name the matches answer to, for offering completions. The group names any match belongs to come first, because a completer offers the list in order and a group name that ties on score would otherwise sit behind a thing's own. Which groups answer follows from what the query kept, so one narrowed to what fights offers no `pickup`.

      Returns: table. A list of names.

### Functions

- [lua]`trx.query.narrowing(make)`  
  Builds a narrowing method for a domain's query type out of a predicate factory. `trx.items` and `trx.objects` declare their own filters with it.

  Parameters:
  - **`make`** (function). Called with the method's own arguments, returning a `predicate(id, handle)`.

  Returns: function. The method to declare as an `impl`.

- [lua]`trx.query.new(domain, class)`  
  Builds the identity query over a domain, as an instance of that domain's query type. `trx.objects` and `trx.items` call this to make the query a script reaches through `trx.objects.query` and `trx.items.query`.

  Parameters:
  - **`domain`** (table). What the query runs against: `enumerate`, `id_of`, `searchable`, and an optional `names_of`/`default_names_of` name layer. See the module source.
  - **`class`** (table). The query type the domain's narrowings were declared on.

  Returns: Query. The identity query, matching everything until narrowed.
