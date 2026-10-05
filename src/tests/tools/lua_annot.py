#!/usr/bin/env python3
"""Unit tests for tools/shared/luaannot. No engine, no binary, no game data."""

from __future__ import annotations

import tempfile
import textwrap
import unittest
from pathlib import Path

import helper  # noqa: F401
from shared.luaannot import (
    AnnotationError,
    finish,
    catalog_names,
    parse_files,
    parse_type,
    signatures,
    to_lua,
)

MODULE = '''\
local h = require("trx.internal.helpers")

---Things module.
---@trx.module 4 Things
---@class (exact) trx.things
---@field power trx.things.State How strong it is.
local M = h.module("things")

---A thing's number.
---@trx.base 1
---@alias trx.things.Num integer

---A length.
---@trx.unit world units, in world units
---@alias trx.things.Distance integer

---A state.
---@enum trx.things.State
local State = {
  ---It is off.
  OFF = h.IntegerConstant,
  ---It is
  ---on.
  ON = h.IntegerConstant,
}
M.State = h.enum("things.State", "THING_STATE", State)

---@class trx.things.poke.opts
---@field force? integer How hard.

---A thing.
---@class (exact) trx.things.Thing
---@trx.readonly num, glow
---@field num trx.things.Num Its number.
---@field hp integer Health.
---@field glow integer How bright it is.
local Thing = h.handle("things.Thing", "THING", {
  fields = { hp = "hp", num = "num" },
  writable = { "hp" },
  extensions = { glow = function(t) return 1 end },
})

---Pokes it.
---@param opts? trx.things.poke.opts How.
---@return boolean # Whether it gave.
function Thing:poke(opts) end

---Every thing.
---@type table<trx.things.Num, trx.things.Thing?>
M.all = h.container("things.all", { base = 1, get = print, count = print })

---Finds things.
---
---```lua
---trx.things.find("a", 2)
---```
---@param name string|integer What to look for.
---@param ... trx.things.Num More of them.
---@return trx.things.Thing[] # The things.
---@return integer # How many.
function M.find(name, ...) end

---Stops.
---@type fun(force?: integer)
M.stop = print
'''


def parsed(source: str = MODULE) -> dict:
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "things.lua"
        path.write_text(textwrap.dedent(source), encoding="utf-8")
        return parse_files([path])


def one(surface: dict, category: str, path: str) -> dict:
    key = "path" if category != "modules" else "name"
    return next(e for e in surface[category] if e[key] == path)


class ParseTests(unittest.TestCase):
    def test_module_takes_its_order_title_and_properties(self):
        surface = parsed()
        module = one(surface, "modules", "things")
        self.assertEqual(module["order"], 4)
        self.assertEqual(module["title"], "Things")
        prop = one(surface, "properties", "things.power")
        self.assertEqual(prop["type"], "things.State")
        self.assertEqual(prop["description"], "How strong it is.")

    def test_aliases_are_numbers_and_units(self):
        surface = parsed()
        self.assertEqual(one(surface, "numbers", "things.Num")["base"], 1)
        unit = one(surface, "units", "things.Distance")
        self.assertEqual(unit["type"], "integer")
        self.assertEqual(unit["spellings"], ["world units", "in world units"])

    def test_enum_reads_its_documentation_table(self):
        enum = one(parsed(), "enums", "things.State")
        docs = {v["name"]: v["description"] for v in enum["values"]}
        self.assertEqual(docs, {"OFF": "It is off.", "ON": "It is on."})

    def test_handle_collects_fields_and_methods(self):
        thing = one(parsed(), "types", "things.Thing")
        self.assertTrue(thing["handle"])
        self.assertEqual([f["name"] for f in thing["fields"]], ["hp", "num"])
        poke = thing["methods"][0]
        self.assertEqual(poke["name"], "poke")
        opts = poke["params"][0]
        self.assertTrue(opts["optional"])
        self.assertEqual(opts["fields"][0]["name"], "force")
        self.assertEqual(poke["returns"]["type"], "boolean")

    def test_function_takes_unions_varargs_examples_and_several_returns(self):
        find = one(parsed(), "functions", "things.find")
        self.assertEqual(find["description"], "Finds things.")
        self.assertEqual(find["examples"], ['trx.things.find("a", 2)'])
        self.assertEqual(find["params"][0]["type"], ["string", "integer"])
        self.assertEqual(find["params"][1]["name"], "...")
        self.assertEqual(find["returns"][0], {
            "type": "things.Thing",
            "list": True,
            "description": "The things.",
        })

    def test_function_type_owns_what_follows_its_parameters(self):
        for text in ("fun(x: integer): boolean?", "fun(): string[]", "fun(): a|nil"):
            spec = parse_type(text)
            self.assertEqual(spec["type"], "function", text)
            self.assertNotIn("nullable", spec, text)
            self.assertNotIn("list", spec, text)
        self.assertTrue(parse_type("(fun(x: integer))?")["nullable"])

    def test_method_returning_self_returns_its_class(self):
        surface = parsed(MODULE.replace(
            "function Thing:poke(opts) end",
            "function Thing:poke(opts) end\n\n---Chains.\n---@return self\nfunction Thing:again() end",
        ))
        method = next(
            m for m in one(surface, "types", "things.Thing")["methods"] if m["name"] == "again"
        )
        self.assertEqual(method["returns"]["type"], "things.Thing")

    def test_function_typed_assignment(self):
        stop = one(parsed(), "functions", "things.stop")
        self.assertEqual(stop["params"], [
            {"name": "force", "type": "integer", "optional": True}
        ])

    def test_container_is_typed_by_its_table(self):
        container = parsed()["containers"][0]
        self.assertEqual(container["member"], "all")
        self.assertEqual(container["key"], {"type": "things.Num"})
        self.assertEqual(
            container["value"], {"type": "things.Thing", "nullable": True}
        )

    def test_annotations_say_what_takes_a_write(self):
        surface = parsed()
        finish(surface)
        enum = one(surface, "enums", "things.State")
        self.assertEqual(enum["count"], 2)
        self.assertEqual(enum["values"][1], {"name": "ON", "description": "It is on."})
        thing = one(surface, "types", "things.Thing")
        self.assertEqual(
            {f["name"]: f["writable"] for f in thing["fields"]},
            {"hp": True, "num": False},
        )
        self.assertEqual([x["name"] for x in thing["extensions"]], ["glow"])
        self.assertTrue(one(surface, "properties", "things.power")["writable"])
        self.assertTrue(surface["containers"][0]["countable"])

    def test_readonly_must_name_a_field(self):
        with self.assertRaises(AnnotationError):
            parsed('''\
            local h = require("trx.internal.helpers")
            ---Things.
            ---@trx.module 4
            ---@class (exact) trx.things
            ---@trx.readonly ghost
            ---@field power integer How strong.
            local M = h.module("things")
            ''')

    def test_signatures_hold_what_strict_mode_checks(self):
        sigs = signatures(parsed())
        self.assertEqual(sigs["methods"]["things.Thing.poke"], [{
            "name": "opts",
            "type": "table",
            "optional": True,
            "fields": [{"name": "force", "type": "integer", "optional": True}],
        }])
        self.assertEqual(sigs["types"]["things.State"], "integer")
        self.assertEqual(sigs["types"]["things.Distance"], "integer")
        self.assertIn('["things.find"]', to_lua(sigs))

    def test_annotation_on_unknown_code_is_an_error(self):
        with self.assertRaises(AnnotationError):
            parsed('''\
            ---Floats.
            ---@param x integer
            print(1)
            ''')

    def test_annotated_local_is_private(self):
        surface = parsed(MODULE + '''
---@type table<string, integer>
local seen = {}

---@param x integer
local function twice(x)
  return 2 * x
end
''')
        paths = [f["path"] for f in surface["functions"]]
        self.assertNotIn("things.twice", paths)

    def test_api_tag_on_a_local_is_an_error(self):
        with self.assertRaises(AnnotationError):
            parsed(MODULE + '''
---@trx.value 1
local ONE = 1
''')

    def test_handle_must_be_declared_as_its_class(self):
        with self.assertRaises(AnnotationError):
            parsed('''\
            local h = require("trx.internal.helpers")
            ---@class trx.things.Other
            local T = h.handle("things.Thing", "THING", {})
            ''')


EXTENDED = """\
local h = require("trx.internal.helpers")

---Widgets module.
---@trx.module 7
---@class (exact) trx.gizmos: trx.gizmos.Self
local M = h.module("gizmos")

---Indexing the module reaches a gizmo.
---@type table<trx.gizmos.Num|string, trx.gizmos.Self?>
h.container("gizmos", { base = 0, get = print }, M)

---@alias trx.gizmos.Num integer

---Every gizmo kind.
---@trx.bulk
---@trx.catalog weapons.def LGT_
---@alias trx.gizmos.kinds integer

---@type table<string, trx.gizmos.kinds>
M.kinds = h.catalog("gizmos.kinds", 3)

---A point.
---@trx.record
---@class trx.gizmos.Point
---@field x integer Across.
---@field y? integer Down.
---@trx.default y 0

---The gizmo itself.
---@class (exact) trx.gizmos.Self
---@field hp integer Health.
local Self = h.handle("gizmos.Self", "GIZMO", { fields = { hp = "hp" } })

---A base query.
---@class (exact) trx.gizmos.Query
---@operator bor(trx.gizmos.Query): trx.gizmos.Query
---@trx.operator bor Either.
local Query = h.class("gizmos.Query")

---Narrows it.
---@return trx.gizmos.Query # It.
function Query:near() end

---A derived query.
---@class (exact) trx.gizmos.SelfQuery: trx.gizmos.Query
local SelfQuery = h.class("gizmos.SelfQuery", { extends = "gizmos.Query" })

---Logs a line. Calling the group logs it plainly.
---@class (exact) trx.gizmos.log
---@param message any What to log.
M.log = h.namespace("gizmos.log", print)

---Logs loudly.
---@param message any What to log.
function M.log.loud(message) end

---@class (exact) trx.gizmos.rules
---@trx.implicit
---@field max integer How many.
M.rules = h.namespace("gizmos.rules")

---The most there can be.
---@type integer
---@trx.value 8
M.MAX = h.const("gizmos.MAX", 8)

---What this build calls itself.
---@type string
---@trx.sample "v1.0"
M.NAME = h.const("gizmos.NAME", "real")

---How fast.
---@enum trx.gizmos.Speed
local Speed = {
  ---Slowly.
  SLOW = h.IntegerConstant,
  ---Quickly.
  FAST = h.IntegerConstant,
}
M.Speed = h.enum("gizmos.Speed", "GIZMO_SPEED", Speed)

---Runs on every tick.
---@param callback fun(frame: integer) The function to run.
---@param speed? trx.gizmos.Speed How often.
---@trx.arg callback.frame The frame number.
---@trx.default speed trx.gizmos.Speed.FAST
function M.on_tick(callback, speed) end
"""

ADDED = """\
local h = require("trx.internal.helpers")
local log = trx.gizmos.log

---Logs quietly.
---@param message any What to log.
function log.quiet(message) end
"""


class ContinuationTests(unittest.TestCase):
    def test_tag_description_continues_on_indented_lines(self):
        surface = parsed('''        local h = require("trx.internal.helpers")

        ---Things.
        ---@trx.module 4
        ---@class (exact) trx.things: trx.things.Self, table<integer, trx.things.Self?>
        ---@field clipboard string The first paragraph,
        ---  wrapped.
        ---
        ---  The second paragraph.
        ---@field plain integer One line.
        local M = h.module("things")

        ---Every thing.
        ---@type table<integer, trx.things.Self?>
        ---@trx.key A thing's number.
        h.container("things", { base = 0, get = print }, M)

        ---Makes one.
        ---@param value any? The first value.
        function M.new(value) end
        ''')
        prop = one(surface, "properties", "things.clipboard")
        self.assertEqual(
            prop["description"],
            "The first paragraph,\nwrapped.\n\nThe second paragraph.",
        )
        self.assertEqual(one(surface, "properties", "things.plain")["description"], "One line.")
        self.assertEqual(one(surface, "modules", "things")["instance_type"], "things.Self")
        self.assertEqual(surface["containers"][0]["key"]["description"], "A thing's number.")
        param = one(surface, "functions", "things.new")["params"][0]
        self.assertTrue(param["nullable"])


class ExtendedTests(unittest.TestCase):
    def setUp(self):
        with tempfile.TemporaryDirectory() as tmp:
            first = Path(tmp) / "gizmos.lua"
            second = Path(tmp) / "gizmos_more.lua"
            first.write_text(EXTENDED, encoding="utf-8")
            second.write_text(ADDED, encoding="utf-8")
            self.surface = parse_files([first, second])

    def test_instance_module_and_module_container(self):
        module = one(self.surface, "modules", "gizmos")
        self.assertEqual(module["instance_type"], "gizmos.Self")
        container = self.surface["containers"][0]
        self.assertNotIn("member", container)
        self.assertEqual(container["key"]["type"], ["gizmos.Num", "string"])

    def test_catalog_takes_its_names_from_its_def_file(self):
        surface = self.surface
        catalog_names(surface, helper.ROOT)
        kinds = one(surface, "enums", "gizmos.kinds")
        self.assertTrue(kinds["bulk"])
        self.assertIn("PISTOLS", kinds["names"])
        self.assertNotIn("LGT_PISTOLS", kinds["names"])
        self.assertEqual(kinds["count"], len(kinds["names"]))

    def test_record_is_a_type_and_not_inlined(self):
        point = one(self.surface, "types", "gizmos.Point")
        self.assertEqual(point["fields"][1]["default"], 0)
        self.assertNotIn("gizmos.Point", signatures(self.surface)["types"])
        self.assertIn("gizmos.Point", signatures(self.surface)["records"])

    def test_class_extends_and_operators(self):
        derived = one(self.surface, "types", "gizmos.SelfQuery")
        self.assertEqual(derived["extends"], "gizmos.Query")
        base = one(self.surface, "types", "gizmos.Query")
        self.assertEqual(base["operators"], [{"name": "bor", "description": "Either."}])
        self.assertFalse(base["handle"])

    def test_namespaces_callable_implicit_and_added_to_elsewhere(self):
        log = one(self.surface, "namespaces", "gizmos.log")
        self.assertTrue(log["callable"])
        self.assertEqual(log["params"][0]["name"], "message")
        rules = one(self.surface, "namespaces", "gizmos.rules")
        self.assertTrue(rules["implicit"])
        self.assertFalse(rules["callable"])
        self.assertEqual(one(self.surface, "properties", "gizmos.rules.max")["type"], "integer")
        one(self.surface, "functions", "gizmos.log.loud")
        one(self.surface, "functions", "gizmos.log.quiet")

    def test_constants_take_a_sample_over_their_value(self):
        surface = self.surface
        finish(surface)
        self.assertEqual(one(surface, "constants", "gizmos.MAX")["value"], 8)
        self.assertEqual(one(surface, "constants", "gizmos.NAME")["value"], "v1.0")

    def test_callback_args_and_defaults_naming_constants(self):
        surface = self.surface
        tick = one(surface, "functions", "gizmos.on_tick")
        frame = tick["params"][0]["params"][0]
        self.assertEqual(frame["description"], "The frame number.")
        sig = signatures(surface)["functions"]["gizmos.on_tick"][1]
        self.assertIn("trx.gizmos.Speed.FAST", to_lua(sig))
        finish(surface)
        self.assertEqual(tick["params"][1]["default"], {"constant": "gizmos.Speed.FAST"})

    def test_members_list_what_each_table_holds(self):
        members = signatures(self.surface)["members"]
        self.assertEqual(
            members["gizmos"],
            ["MAX", "NAME", "Speed", "kinds", "log", "on_tick", "rules"],
        )
        self.assertEqual(members["gizmos.log"], ["loud", "quiet"])

    def test_reaching_an_undeclared_namespace_is_an_error(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "stray.lua"
            path.write_text(ADDED, encoding="utf-8")
            with self.assertRaises(AnnotationError):
                parse_files([path])


NULLABLE = """\
local h = require("trx.internal.helpers")

---Things.
---@trx.module 4
---@class (exact) trx.things
---@field target integer? What it aims at.
local M = h.module("things")

---@class trx.things.base
---@field a integer The first.

---@class trx.things.opts : trx.things.base
---@field b integer The second.

---Aims.
---@param at integer? Where.
---@param name string|nil What.
---@param opts trx.things.base How.
---@param more trx.things.opts And more.
---@trx.default name nil
function M.aim(at, name, opts, more) end
"""


class NullableTests(unittest.TestCase):
    def setUp(self):
        self.sigs = signatures(parsed(NULLABLE))

    def test_nil_in_a_union_makes_the_type_nullable(self):
        self.assertEqual(
            parse_type("string|nil"), {"type": "string", "nullable": True}
        )
        self.assertEqual(
            parse_type("string|integer|nil"),
            {"type": ["string", "integer"], "nullable": True},
        )

    def test_nullable_parameter_is_optional(self):
        at, name, _, _ = self.sigs["functions"]["things.aim"]
        self.assertEqual(at, {"name": "at", "type": "integer", "optional": True})
        self.assertEqual(name["type"], "string")
        self.assertTrue(name["optional"])

    def test_nullable_property_takes_nil(self):
        self.assertEqual(
            self.sigs["properties"]["things.target"],
            {"type": "integer", "optional": True},
        )

    def test_shape_keeps_its_own_name_when_it_extends_another(self):
        _, _, opts, more = self.sigs["functions"]["things.aim"]
        self.assertEqual([f["name"] for f in opts["fields"]], ["a"])
        self.assertEqual([f["name"] for f in more["fields"]], ["a", "b"])

    def test_nil_default_is_written_as_lua(self):
        self.assertIn('["default"] = nil', to_lua(self.sigs))


if __name__ == "__main__":
    unittest.main(verbosity=2)
