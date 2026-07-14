#!/usr/bin/env python3
"""Unit tests for tools/update_lua_docs. No engine, no binary, no game data."""

from __future__ import annotations

import copy
import json
import unittest

from tools_helper import load

docs = load("update_lua_docs")

# A surface with one of everything, so the renderer has to handle each kind.
SURFACE = {
    "modules": [{"name": "things", "order": 4, "description": "Things module."}],
    "types": [
        {
            "path": "things.Widget",
            "description": "A widget.",
            "fields": [
                {
                    "name": "shown",
                    "type": "integer",
                    "writable": True,
                    "description": "Visible value.",
                    "enum": "things.State",
                },
                {
                    "name": "locked",
                    "type": "integer",
                    "writable": False,
                    "description": "Cannot be written.",
                },
            ],
            "methods": [
                {
                    "name": "poke",
                    "description": "Pokes it.",
                    "params": [{"name": "force", "type": "integer"}],
                    "returns": {"type": "boolean"},
                }
            ],
            "extensions": [
                {"name": "derived", "type": "integer", "description": "Computed."}
            ],
        }
    ],
    "enums": [
        {
            "path": "things.State",
            "description": "A state.",
            "values": [
                {"name": "OFF", "value": 0, "description": "It is off."},
                {"name": "ON", "value": 1, "description": "It is on."},
                {"name": "BROKEN", "value": 7, "description": "It is broken."},
            ],
        }
    ],
    "constants": [
        {"path": "things.WALL_L", "value": 1024, "description": "One sector."}
    ],
    "properties": [
        {
            "path": "things.pos",
            "type": "vec3",
            "writable": False,
            "description": "Where it is.",
        },
        {
            "path": "things.power",
            "type": "integer",
            "writable": True,
            "description": "How strong it is.",
            "enum": "things.State",
        },
    ],
    "functions": [
        {
            "path": "things.spawn",
            "description": "Spawns a thing.",
            "params": [
                {"name": "id", "type": "integer", "enum": "catalog.objects"},
                {
                    "name": "angle",
                    "type": "integer",
                    "optional": True,
                    "default": 0,
                    "description": "Facing.",
                },
            ],
            "returns": {"type": "Widget", "nullable": True},
            "examples": ["local w = trx.things.spawn(1)"],
        },
        {
            "path": "things.on_poked",
            "description": "Registers a handler.",
            "params": [
                {
                    "name": "callback",
                    "type": "function",
                    "description": "Called when a thing is poked.",
                    "params": [
                        {
                            "name": "strength",
                            "type": "integer",
                            "description": "How hard.",
                        },
                        {
                            "name": "id",
                            "type": "integer",
                            "enum": "catalog.objects",
                            "description": "What was poked.",
                        },
                    ],
                }
            ],
            "returns": {"type": "integer"},
        },
    ],
}


class TestLuaDocs(unittest.TestCase):
    def test_every_member_of_the_surface_reaches_the_page(self):
        """Every kind of member in the registry reaches the page.

        Fields, computed members and methods all render, so nothing declared in
        the registry can go missing from the reference.
        """
        page = docs.render_page(SURFACE["modules"][0], SURFACE)

        for spec in SURFACE["types"]:
            for kind in ("fields", "methods", "extensions"):
                for member in spec[kind]:
                    self.assertIn(
                        member["name"],
                        page,
                        f"{kind[:-1]} '{member['name']}' is missing from the page",
                    )
        for func in SURFACE["functions"]:
            self.assertIn(func["path"], page)
        for spec in SURFACE["enums"]:
            for value in spec["values"]:
                self.assertIn(
                    value["name"],
                    page,
                    f"constant '{value['name']}' is missing from the page",
                )

    def test_enum_constants_are_rendered_with_their_values(self):
        """An enum's constants render with their names and values.

        An enum reaches the docs only through the registry: the dump cannot see
        one pushed straight onto a module table.
        """
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        self.assertIn("### Enums", page)
        self.assertIn("`trx.things.State.OFF` = `0`", page)
        self.assertIn("`trx.things.State.ON` = `1`", page)
        # Not 2. The value comes from C, gaps and all.
        self.assertIn("`trx.things.State.BROKEN` = `7`", page)
        self.assertIn("It is broken.", page)

    def test_enum_cross_references_are_rendered(self):
        """A field names the enum it accepts by path."""
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        shown = next(line for line in page.splitlines() if "`shown`" in line)
        self.assertIn("Compare against `trx.things.State`.", shown)
        # Params too, and the target need not be a registry enum: the catalog
        # tables are CSV-driven, and a pointer at them is still worth rendering.
        param = next(line for line in page.splitlines() if "**`id`**" in line)
        self.assertIn("Compare against `trx.catalog.objects`.", param)

        # A member with no enum must not grow the sentence.
        locked = next(line for line in page.splitlines() if "`locked`" in line)
        self.assertNotIn("Compare against", locked)

    def test_a_return_value_cross_references_its_enum(self):
        """A function that hands back an id is as worth cross-referencing as a
        param that takes one."""
        surface = copy.deepcopy(SURFACE)
        surface["functions"][0]["returns"] = {
            "type": "integer",
            "enum": "catalog.music",
            "description": "The track.",
        }
        page = docs.render_page(surface["modules"][0], surface)
        returns = next(
            line
            for line in page.splitlines()
            if "Returns:" in line and "The track." in line
        )
        self.assertIn("Compare against `trx.catalog.music`.", returns)

    def test_read_only_members_are_marked(self):
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        locked = next(
            line for line in page.splitlines() if "`locked`" in line
        )
        shown = next(line for line in page.splitlines() if "`shown`" in line)
        self.assertIn("read-only", locked)
        self.assertNotIn("read-only", shown)

    def test_optional_params_are_bracketed_with_their_default(self):
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        self.assertIn("things.spawn(id, [angle])", page)
        self.assertIn("default `0`", page)

    def test_an_enum_default_reads_as_the_constant(self):
        """The default is stored as the constant's value, because that is what
        the wrapper substitutes. It has to read as the constant."""
        surface = copy.deepcopy(SURFACE)
        surface["functions"][0]["params"][1].update(
            {"default": 1, "enum": "things.State"}
        )
        docs.ENUM_CONSTANTS.clear()
        docs.ENUM_CONSTANTS["things.State"] = {0: "OFF", 1: "ON", 7: "BROKEN"}

        page = docs.render_page(surface["modules"][0], surface)
        self.assertIn("default `trx.things.State.ON`", page)
        self.assertNotIn("default `1`", page)
        docs.ENUM_CONSTANTS.clear()

    def test_several_returns_are_all_rendered(self):
        """A Lua function is free to hand back more than one value, and the
        reference has to say what each of them is."""
        surface = copy.deepcopy(SURFACE)
        surface["functions"][0]["returns"] = [
            {"type": "vec3", "nullable": True, "description": "Where."},
            {"type": "integer", "description": "Which room."},
        ]

        page = docs.render_page(surface["modules"][0], surface)
        self.assertIn("vec3 or `nil`. Where.", page)
        self.assertIn("integer. Which room.", page)

    def test_an_indexable_module_is_described(self):
        """Indexing lives on a metatable, which pairs() never sees, so the page
        can only describe it because the container is declared."""
        surface = copy.deepcopy(SURFACE)
        surface["containers"] = [
            {
                "module": "things",
                "description": "Indexing reaches a thing.",
                "key": {"type": "integer", "description": "1-based."},
                "value": {"type": "Widget", "nullable": True},
                "countable": True,
            }
        ]

        page = docs.render_page(surface["modules"][0], surface)
        self.assertIn("### Indexing", page)
        self.assertIn("**`trx.things[key]`** (Widget or `nil`). 1-based.", page)
        self.assertIn("**`#trx.things`** (integer).", page)

    def test_a_module_with_no_container_gets_no_indexing_section(self):
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        self.assertNotIn("### Indexing", page)

    def test_a_callbacks_own_arguments_are_rendered(self):
        """A function-typed parameter renders the arguments it is called with,
        indented under the parameter itself."""
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        self.assertIn("Called with:", page)
        strength = next(
            line for line in page.splitlines() if "**`strength`**" in line
        )
        self.assertIn("How hard.", strength)
        # Indented under the callback, not hoisted to the function's own params.
        callback = next(
            line for line in page.splitlines() if "**`callback`**" in line
        )
        self.assertGreater(
            len(strength) - len(strength.lstrip()),
            len(callback) - len(callback.lstrip()),
        )
        # A callback argument gets the same enum cross-reference a param does.
        arg = next(line for line in page.splitlines() if "**`id`**" in line)
        self.assertIn("Compare against `trx.catalog.objects`.", arg)

    def test_a_param_with_no_callback_args_stays_flat(self):
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        angle = next(line for line in page.splitlines() if "**`angle`**" in line)
        self.assertNotIn("Called with:", angle)

    def test_nullable_returns_say_so(self):
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        self.assertIn("Widget or `nil`", page)

    def test_examples_are_rendered_as_lua_blocks(self):
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        self.assertIn("```lua", page)
        self.assertIn("trx.things.spawn(1)", page)

    def test_module_properties_are_rendered(self):
        """A module property reaches the page, marked read-only when it has no
        setter."""
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        self.assertIn("### Properties", page)

        pos = next(line for line in page.splitlines() if "`trx.things.pos`" in line)
        self.assertIn("Where it is.", pos)
        self.assertIn("read-only", pos)

        power = next(line for line in page.splitlines() if "`trx.things.power`" in line)
        self.assertNotIn("read-only", power)
        self.assertIn("Compare against `trx.things.State`.", power)

    def test_a_module_may_override_its_title(self):
        """Capitalizing the module name gives "Log" and "Assault"; the pages are
        Logging and Assault course."""
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        self.assertIn("title: Things", page)

        surface = copy.deepcopy(SURFACE)
        surface["modules"][0]["title"] = "Thingamabobs"
        page = docs.render_page(surface["modules"][0], surface)
        self.assertIn("title: Thingamabobs", page)
        self.assertIn("## Thingamabobs module", page)

    def test_a_namespace_reaches_the_page_whether_or_not_it_is_callable(self):
        """A namespace gets its own bullet either way; only a callable one
        renders as a call."""
        surface = copy.deepcopy(SURFACE)
        surface["namespaces"] = [
            {"path": "things.log", "description": "Logs it.", "callable": True},
            {"path": "things.stats", "description": "Counts it.", "callable": False},
        ]
        page = docs.render_page(surface["modules"][0], surface)
        self.assertIn("Logs it.", page)
        self.assertIn("Counts it.", page)

        # Callable renders as a call; a plain group does not pretend to be one.
        self.assertIn("`trx.things.log()`", page)
        self.assertIn("`trx.things.stats`", page)
        self.assertNotIn("`trx.things.stats()`", page)

    def test_a_multi_paragraph_description_stays_inside_its_list_item(self):
        """Every paragraph of a description stays indented inside its list item."""
        surface = copy.deepcopy(SURFACE)
        surface["functions"][0]["description"] = "First para.\n\nSecond para."
        page = docs.render_page(surface["modules"][0], surface)
        second = next(line for line in page.splitlines() if "Second para." in line)
        self.assertTrue(
            second.startswith("  "), f"paragraph escaped its list item: {second!r}"
        )

    def test_a_bulk_enum_lists_its_names_but_not_its_numbers(self):
        """A bulk enum renders a count and a folded name list, and no values."""
        surface = copy.deepcopy(SURFACE)
        surface["enums"][0].update(
            {"bulk": True, "count": 3, "values": [], "names": ["BROKEN", "OFF", "ON"]}
        )
        page = docs.render_page(surface["modules"][0], surface)

        self.assertIn("`trx.things.State` - 3 names", page)
        self.assertIn("`BROKEN`, `OFF`, `ON`", page)
        self.assertIn("Click here to see a list of all symbols.", page)

        # No bullet per constant, and no values.
        self.assertNotIn("`trx.things.State.OFF` = `0`", page)

    def test_rendering_is_stable(self):
        """Two runs must agree, or --check would flag spurious drift forever."""
        a = docs.render_page(SURFACE["modules"][0], SURFACE)
        b = docs.render_page(SURFACE["modules"][0], SURFACE)
        self.assertEqual(a, b)

    def test_a_declared_member_with_no_description_is_reported(self):
        """A member with no description still reaches scripts; it just renders
        blank."""
        self.assertEqual(docs.undocumented(SURFACE), [])

        cases = [
            (("functions", 0), "things.spawn"),
            (("constants", 0), "things.WALL_L"),
            (("enums", 0, "values", 1), "things.State.ON"),
            (("types", 0, "fields", 0), "things.Widget.shown"),
            (("types", 0, "methods", 0), "things.Widget.poke"),
            (("types", 0, "extensions", 0), "things.Widget.derived"),
            (("properties", 0), "things.pos"),
        ]
        for path, expected in cases:
            with self.subTest(member=expected):
                surface = copy.deepcopy(SURFACE)
                target = surface
                for key in path:
                    target = target[key]
                del target["description"]
                self.assertEqual(docs.undocumented(surface), [expected])

    def test_dump_extracts_the_json_from_a_noisy_stream(self):
        """The JSON payload is picked out of a stream that also carries logs."""
        stream = 'INF | starting\nDBG | loading\n{"modules": []}\n'
        payload = next(
            (line for line in stream.splitlines() if line.startswith("{")), None
        )
        self.assertEqual(json.loads(payload), {"modules": []})

if __name__ == "__main__":
    unittest.main(verbosity=2)
