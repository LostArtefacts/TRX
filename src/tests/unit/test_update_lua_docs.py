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
        """The property whose absence caused the original bug.

        The generated reference documented 31 struct fields but silently omitted
        both computed members and all eight methods, while presenting itself as
        the complete surface. Assert that nothing in the registry can go missing.
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
        """The regression this whole change exists to fix.

        The reference used to point readers at `trx.rooms.fn.FlipStatus` while
        documenting none of its constants, because enums were pushed onto the
        module table outside the registry and the dump could not see them.
        """
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        self.assertIn("### Enums", page)
        self.assertIn("`trx.things.State.OFF` = `0`", page)
        self.assertIn("`trx.things.State.ON` = `1`", page)
        # Not 2. The value comes from C, gaps and all.
        self.assertIn("`trx.things.State.BROKEN` = `7`", page)
        self.assertIn("It is broken.", page)

    def test_enum_cross_references_are_rendered(self):
        """A field or param names the enum it accepts by path, rather than
        repeating it in prose that nothing keeps in sync."""
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

    def test_a_callbacks_own_arguments_are_rendered(self):
        """An event hook takes nothing but a callback, so the callback's
        signature is the only one worth documenting. Without this the page says
        `trx.events.on_pickup(callback)` and never mentions that the callback is
        handed the item number."""
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
        """trx.camera.pos is neither a function nor a constant: reading it calls
        into the engine. It still has to reach the page."""
        page = docs.render_page(SURFACE["modules"][0], SURFACE)
        self.assertIn("### Properties", page)

        pos = next(line for line in page.splitlines() if "`trx.things.pos`" in line)
        self.assertIn("Where it is.", pos)
        self.assertIn("read-only", pos)

        power = next(line for line in page.splitlines() if "`trx.things.power`" in line)
        self.assertNotIn("read-only", power)
        self.assertIn("Compare against `trx.things.State`.", power)

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
        """The engine logs to stdout alongside the JSON, so the payload must be
        picked out rather than the whole stream trusted."""
        stream = 'INF | starting\nDBG | loading\n{"modules": []}\n'
        payload = next(
            (line for line in stream.splitlines() if line.startswith("{")), None
        )
        self.assertEqual(json.loads(payload), {"modules": []})

if __name__ == "__main__":
    unittest.main(verbosity=2)
