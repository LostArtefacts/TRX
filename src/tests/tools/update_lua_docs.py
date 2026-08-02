#!/usr/bin/env python3
"""Unit tests for tools/lint/gen/lua_docs. No engine, no binary, no game data."""

from __future__ import annotations

import copy
import json
import unittest

from helper import load

docs = load("lint/gen/lua_docs")


# A catalog as the dump carries one: an enum of names, declared elsewhere.
CATALOG = {
    "path": "catalog.objects",
    "description": "Every object the engine knows.",
    "bulk": True,
    "count": 2,
    "names": ["WOLF", "BEAR"],
}


def rendered(surface, module=None):
    """A page, rendered the way the tool renders one: dump read in first.

    The tables are cleared first, so one test's surface is not still standing
    when the next one renders.
    """
    for table in (
        docs.NUMBERS,
        docs.UNITS,
        docs.ANCHORS,
        docs.TYPE_PATHS,
        docs.STANDS_FOR,
        docs.KEYED_BY,
        docs.CONSTANTS,
        docs.ENUM_PATHS,
        docs.ENUM_CONSTANTS,
        docs.ALIASES,
    ):
        table.clear()
    docs.read(surface)
    return docs.render_page(module or surface["modules"][0], surface)

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
                    "type": "things.State",
                    "writable": True,
                    "description": "Visible value.",
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
                    "params": [
                        {
                            "name": "force",
                            "type": "integer",
                            "description": "How hard.",
                        }
                    ],
                    "returns": {"type": "boolean", "description": "Whether it gave."},
                }
            ],
            "extensions": [
                {"name": "derived", "type": "integer", "description": "Computed."}
            ],
        }
    ],
    "enums": [
        {
            "path": "catalog.objects",
            "description": "Every object the engine knows.",
            "bulk": True,
            "count": 2,
            "names": ["WOLF", "BEAR"],
        },
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
            "type": "things.State",
            "writable": True,
            "description": "How strong it is.",
        },
    ],
    "functions": [
        {
            "path": "things.spawn",
            "description": "Spawns a thing.",
            "params": [
                {"name": "id", "type": "catalog.objects"},
                {
                    "name": "angle",
                    "type": "integer",
                    "optional": True,
                    "default": 0,
                    "description": "Facing.",
                },
            ],
            "returns": {
                "type": "Widget",
                "nullable": True,
                "description": "The thing, or `nil` where none was free.",
            },
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
                            "type": "catalog.objects",
                            "description": "What was poked.",
                        },
                    ],
                }
            ],
            "returns": {"type": "integer", "description": "The handler's id."},
        },
    ],
}


class TestLuaDocs(unittest.TestCase):
    def test_every_member_of_the_surface_reaches_the_page(self):
        """Every kind of member in the registry reaches the page.

        Fields, computed members and methods all render, so nothing declared in
        the registry can go missing from the reference.
        """
        page = rendered(SURFACE)

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
            for value in spec.get("values") or []:
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
        page = rendered(SURFACE)
        self.assertIn("### Enums", page)
        self.assertIn("`trx.things.State.OFF` = `0`", page)
        self.assertIn("`trx.things.State.ON` = `1`", page)
        # Not 2. The value comes from C, gaps and all.
        self.assertIn("`trx.things.State.BROKEN` = `7`", page)
        self.assertIn("It is broken.", page)

    def test_a_number_that_names_an_enum_reads_as_that_enum(self):
        """The enum a declaration points at is the type it reads as.

        `integer` says what the engine passes; the enum says what the value
        means and lists what it can be, a link away.
        """
        surface = copy.deepcopy(SURFACE)
        surface["enums"].append(CATALOG)
        page = rendered(surface)
        shown = next(line for line in page.splitlines() if "`shown`" in line)
        self.assertIn("**`shown`**: [trx.things.State](#things.State).", shown)
        # A catalog is an enum too, and the pages it links to sit elsewhere.
        param = next(line for line in page.splitlines() if "**`id`**" in line)
        self.assertIn(
            "**`id`** ([trx.catalog.objects](CATALOG.md#catalog.objects))",
            param,
        )

        # A member that points nowhere reads as the type it declares.
        locked = next(line for line in page.splitlines() if "`locked`" in line)
        self.assertIn("**`locked`**: integer.", locked)

    def test_a_reference_reads_short_only_among_members_of_one_thing(self):
        """A path in prose is written in full and reads as far as it has to.

        Between members of one type or group the name alone is enough, and the
        module and the path it hangs off are noise. Anywhere else the reader
        needs the whole path to know what is meant.
        """
        surface = copy.deepcopy(SURFACE)
        surface["types"][0]["fields"][1]["description"] = (
            "Set `trx.things.Widget.shown` instead, and see `trx.things.spawn`."
        )
        surface["functions"][0]["description"] = (
            "Spawns a thing, of the shape `trx.things.Widget`."
        )
        docs.ANCHORS.update(docs.anchors_of(surface))
        page = rendered(surface)

        locked = next(line for line in page.splitlines() if "`locked`" in line)
        self.assertIn("[`shown`](#things.Widget.shown)", locked)
        self.assertIn("[`trx.things.spawn`](#things.spawn)", locked)

        spawn = next(line for line in page.splitlines() if "Spawns a thing" in line)
        self.assertIn("[`trx.things.Widget`](#things.Widget)", spawn)

    def test_a_method_reference_reads_as_a_call(self):
        """A method is pointed at the way a script writes it, with a colon."""
        surface = copy.deepcopy(SURFACE)
        surface["functions"][0]["description"] = "As `trx.things.Widget:poke`."
        docs.ANCHORS.update(docs.anchors_of(surface))
        page = rendered(surface)
        self.assertIn(
            "[`trx.things.Widget:poke`](#things.Widget.poke)",
            page,
        )

    def test_a_member_reached_through_its_module_links_to_its_declaration(self):
        """A module that stands for one thing answers for its members.

        A script writes `trx.lara.air` rather than the type's own path, and the
        anchor is written once, where the type declares the member.
        """
        surface = copy.deepcopy(SURFACE)
        surface["modules"][0]["instance_type"] = "things.Widget"
        surface["functions"][0]["description"] = "As `trx.things.shown`."
        docs.ANCHORS.update(docs.anchors_of(surface))
        docs.ALIASES["things.shown"] = "things.Widget.shown"
        page = rendered(surface)
        self.assertIn("[`trx.things.shown`](#things.Widget.shown)", page)

    def test_a_computed_property_links_what_it_names(self):
        """A computed member's description is prose like any other."""
        surface = copy.deepcopy(SURFACE)
        surface["types"][0]["extensions"][0]["description"] = (
            "Computed from `trx.things.Widget.shown`."
        )
        docs.ANCHORS.update(docs.anchors_of(surface))
        page = rendered(surface)
        derived = next(line for line in page.splitlines() if "`derived`" in line)
        self.assertIn("[`shown`](#things.Widget.shown)", derived)

    def test_a_page_of_the_manual_is_named_from_the_root(self):
        """A description that points at a page names it where it lives.

        Where the link lands is the generated page's business, and a page that
        is not there is a broken link nobody would notice.
        """
        surface = copy.deepcopy(SURFACE)
        surface["functions"][0]["description"] = (
            "Spawns it. See [Objects](docs/trx/OBJECTS.md)."
        )
        page = rendered(surface)
        self.assertIn("See [Objects](../../OBJECTS.md).", page)

        surface["functions"][0]["description"] = "See [Nope](docs/trx/NOPE.md)."
        with self.assertRaises(SystemExit):
            rendered(surface)

    def test_a_number_is_named_rather_than_described_again(self):
        """A declaration that holds a named number names it.

        What a room number is, and where it counts from, is the number's own
        business. What holds one says only what is its own, so the sentence is
        written once however many places mean it.
        """
        surface = copy.deepcopy(SURFACE)
        surface["numbers"] = [
            {
                "path": "things.Num",
                "description": "Thing number, as the level numbers them.",
                "base": 0,
            }
        ]
        surface["functions"][0]["params"] = [
            {"name": "num", "type": "things.Num"},
            {
                "name": "other",
                "type": "things.Num",
                "description": "The one that broke.",
            },
        ]
        page = rendered(surface)

        # The number renders among the structures, and says what it is once.
        self.assertIn("### Structures", page)
        self.assertIn(docs.anchor("things.Num"), page)
        definition = next(
            line
            for line in page.splitlines()
            if "Thing number, as the level numbers them." in line
        )
        self.assertIn("Counted from 0.", definition)

        # What holds one names it as its type, and carries neither the
        # sentence nor the base.
        num = next(line for line in page.splitlines() if "**`num`**" in line)
        self.assertIn("**`num`** ([trx.things.Num](#things.Num))", num)
        self.assertNotIn("Thing number", num)
        self.assertNotIn("Counted from", num)

        other = next(line for line in page.splitlines() if "**`other`**" in line)
        self.assertIn("([trx.things.Num](#things.Num)). The one that broke.", other)

    def test_a_number_described_away_from_itself_is_reported(self):
        """The three ways a named number gets copied instead of named."""
        surface = copy.deepcopy(SURFACE)
        surface["numbers"] = [
            {
                "path": "things.Num",
                "description": "Thing number, as the level numbers them.",
                "base": 0,
            }
        ]
        surface["functions"][0]["params"] = [
            {"name": "num", "type": "things.Num"},
        ]
        docs.read(surface)
        self.assertEqual(docs.respelled(surface), [])

        # Its words, written somewhere that does not name it.
        said_again = copy.deepcopy(surface)
        said_again["functions"][1]["description"] = (
            "Takes a thing number, as the level numbers them."
        )
        report = docs.respelled(said_again)
        self.assertEqual(len(report), 1, report)
        self.assertIn("says what `trx.things.Num` says", report[0])

        # A base of its own, beside a number that declares one.
        rebased = copy.deepcopy(surface)
        rebased["functions"][0]["params"][0]["base"] = 1
        self.assertIn("counts from its own base", docs.respelled(rebased)[0])

        # And a number nothing holds at all.
        unheld = copy.deepcopy(surface)
        unheld["functions"][0]["params"][0]["type"] = "integer"
        self.assertIn("a number nothing holds", docs.respelled(unheld)[-1])

    def test_a_unit_written_out_instead_of_named_is_reported(self):
        """A unit says what a value of it is measured in, once."""
        surface = copy.deepcopy(SURFACE)
        surface["units"] = [
            {
                "path": "things.Distance",
                "description": "A length in the units the engine measures the world in.",
                "type": "integer",
                "spellings": ["world units"],
            }
        ]
        surface["functions"][0]["params"] = [
            {"name": "reach", "type": "things.Distance"},
        ]
        docs.read(surface)
        self.assertEqual(docs.respelled(surface), [])

        # The words, written somewhere that does not name the unit.
        said_again = copy.deepcopy(surface)
        said_again["functions"][1]["description"] = "How far it reaches, in world units."
        report = docs.respelled(said_again)
        self.assertEqual(len(report), 1, report)
        self.assertIn("writes out what `trx.things.Distance` is", report[0])

        # And a unit nothing is measured in.
        unheld = copy.deepcopy(surface)
        unheld["functions"][0]["params"][0]["type"] = "integer"
        self.assertIn("a unit nothing is measured in", docs.respelled(unheld)[-1])

    def test_a_declaration_typed_by_a_unit_needs_no_words(self):
        """What it is, and what it is measured in, is what the type says."""
        self.assertFalse(docs.documented({"name": "reach", "type": "integer"}))
        docs.read(
            {
                **copy.deepcopy(SURFACE),
                "units": [
                    {
                        "path": "things.Distance",
                        "description": "A length.",
                        "type": "integer",
                    }
                ],
            }
        )
        self.assertTrue(docs.documented({"name": "reach", "type": "things.Distance"}))

    def test_a_return_value_cross_references_its_enum(self):
        """A function that hands back an id is as worth cross-referencing as a
        param that takes one."""
        surface = copy.deepcopy(SURFACE)
        surface["functions"][0]["returns"] = {
            "type": "catalog.music",
            "description": "The track.",
        }
        surface["enums"].append({**CATALOG, "path": "catalog.music"})
        page = rendered(surface)
        returns = next(
            line
            for line in page.splitlines()
            if "Returns:" in line and "The track." in line
        )
        self.assertIn(
            "Returns: [trx.catalog.music](CATALOG.md#catalog.music).",
            returns,
        )

    def test_read_only_members_are_marked(self):
        page = rendered(SURFACE)
        locked = next(
            line for line in page.splitlines() if "`locked`" in line
        )
        shown = next(line for line in page.splitlines() if "`shown`" in line)
        self.assertIn("read-only", locked)
        self.assertNotIn("read-only", shown)

    def test_optional_params_are_bracketed_with_their_default(self):
        page = rendered(SURFACE)
        self.assertIn("things.spawn(id, [angle])", page)
        self.assertIn("default `0`", page)

    def test_an_enum_default_reads_as_the_constant(self):
        """The default is stored as the constant's value, because that is what
        the wrapper substitutes. It has to read as the constant."""
        surface = copy.deepcopy(SURFACE)
        surface["functions"][0]["params"][1].update(
            {"default": 1, "type": "things.State"}
        )
        docs.ENUM_CONSTANTS.clear()
        docs.ENUM_CONSTANTS["things.State"] = {0: "OFF", 1: "ON", 7: "BROKEN"}

        page = rendered(surface)
        self.assertIn("default [`trx.things.State.ON`](#things.State)", page)
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

        page = rendered(surface)
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

        page = rendered(surface)
        self.assertIn("### Indexing", page)
        self.assertIn("**`trx.things[key]`** (Widget or `nil`). 1-based.", page)
        self.assertIn("**`#trx.things`** (integer).", page)

    def test_a_module_with_no_container_gets_no_indexing_section(self):
        page = rendered(SURFACE)
        self.assertNotIn("### Indexing", page)

    def test_a_callbacks_own_arguments_are_rendered(self):
        """A function-typed parameter renders the arguments it is called with,
        indented under the parameter itself."""
        surface = copy.deepcopy(SURFACE)
        surface["enums"].append(CATALOG)
        page = rendered(surface)
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
        # A callback argument is typed by the enum it names, as a param is.
        arg = next(line for line in page.splitlines() if "**`id`**" in line)
        self.assertIn(
            "**`id`** ([trx.catalog.objects](CATALOG.md#catalog.objects))",
            arg,
        )

    def test_a_param_with_no_callback_args_stays_flat(self):
        page = rendered(SURFACE)
        angle = next(line for line in page.splitlines() if "**`angle`**" in line)
        self.assertNotIn("Called with:", angle)

    def test_nullable_returns_say_so(self):
        page = rendered(SURFACE)
        self.assertIn("Widget or `nil`", page)

    def test_examples_are_rendered_as_lua_blocks(self):
        page = rendered(SURFACE)
        self.assertIn("```lua", page)
        self.assertIn("trx.things.spawn(1)", page)

    def test_module_properties_are_rendered(self):
        """A module property reaches the page, marked read-only when it has no
        setter."""
        page = rendered(SURFACE)
        self.assertIn("### Properties", page)

        pos = next(line for line in page.splitlines() if "`trx.things.pos`" in line)
        self.assertIn("Where it is.", pos)
        self.assertIn("read-only", pos)

        power = next(line for line in page.splitlines() if "`trx.things.power`" in line)
        self.assertNotIn("read-only", power)
        self.assertIn("([trx.things.State](#things.State))", power)

    def test_a_module_may_override_its_title(self):
        """Capitalizing the module name gives "Log" and "Assault"; the pages are
        Logging and Assault course."""
        page = rendered(SURFACE)
        self.assertIn("title: Things", page)

        surface = copy.deepcopy(SURFACE)
        surface["modules"][0]["title"] = "Thingamabobs"
        page = rendered(surface)
        self.assertIn("title: Thingamabobs", page)
        self.assertIn("## " + docs.anchor("things") + "Thingamabobs module", page)

    def test_a_namespace_reaches_the_page_whether_or_not_it_is_callable(self):
        """A namespace gets its own bullet either way; only a callable one
        renders as a call."""
        surface = copy.deepcopy(SURFACE)
        surface["namespaces"] = [
            {"path": "things.log", "description": "Logs it.", "callable": True},
            {"path": "things.stats", "description": "Counts it.", "callable": False},
        ]
        page = rendered(surface)
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
        page = rendered(surface)
        second = next(line for line in page.splitlines() if "Second para." in line)
        self.assertTrue(
            second.startswith("  "), f"paragraph escaped its list item: {second!r}"
        )

    def test_a_bulk_enum_lists_its_names_but_not_its_numbers(self):
        """A bulk enum renders a count and a folded name list, and no values."""
        surface = copy.deepcopy(SURFACE)
        surface["enums"][1].update(
            {"bulk": True, "count": 3, "values": [], "names": ["BROKEN", "OFF", "ON"]}
        )
        page = rendered(surface)

        self.assertIn("`trx.things.State` - 3 names", page)
        self.assertIn("`BROKEN`, `OFF`, `ON`", page)
        self.assertIn("Click here to see a list of all symbols.", page)

        # No bullet per constant, and no values.
        self.assertNotIn("`trx.things.State.OFF` = `0`", page)

    def test_rendering_is_stable(self):
        """Two runs must agree, or --check would flag spurious drift forever."""
        a = rendered(SURFACE)
        b = rendered(SURFACE)
        self.assertEqual(a, b)

    def test_a_declared_member_with_no_description_is_reported(self):
        """A member with no description still reaches scripts; it just renders
        blank."""
        docs.read(SURFACE)
        self.assertEqual(docs.undocumented(SURFACE), [])

        cases = [
            (("functions", 0), "things.spawn"),
            (("constants", 0), "things.WALL_L"),
            (("enums", 1, "values", 1), "things.State.ON"),
            (("types", 0, "fields", 0), "things.Widget.shown"),
            (("types", 0, "methods", 0), "things.Widget.poke"),
            (("types", 0, "extensions", 0), "things.Widget.derived"),
            (("properties", 0), "things.pos"),
            # An argument and a result are as blank as anything else without
            # words of their own.
            (("functions", 0, "params", 1), "things.spawn(angle)"),
            (("types", 0, "methods", 0, "returns"), "things.Widget.poke returns boolean"),
        ]
        for path, expected in cases:
            with self.subTest(member=expected):
                surface = copy.deepcopy(SURFACE)
                target = surface
                for key in path:
                    target = target[key]
                del target["description"]
                # A member that points somewhere is documented by what it
                # points at, so only one with nothing at all is reported.
                if target.get("type") in ("things.State", "catalog.objects"):
                    target["type"] = "integer"
                docs.read(surface)
                self.assertEqual(docs.undocumented(surface), [expected])

    def test_a_result_holding_several_is_named_by_the_call(self):
        """A parameter still says what it is for; only a result is let off."""
        surface = copy.deepcopy(SURFACE)
        returns = surface["types"][0]["methods"][0]["returns"]
        del returns["description"]
        returns["list"] = True
        docs.read(surface)
        self.assertEqual(docs.undocumented(surface), [])

        surface = copy.deepcopy(SURFACE)
        param = surface["functions"][0]["params"][1]
        del param["description"]
        param["list"] = True
        docs.read(surface)
        self.assertEqual(docs.undocumented(surface), ["things.spawn(angle)"])

    def test_a_type_naming_nothing_stops_the_run(self):
        """The engine emits what it was told; resolving it is this tool's job."""
        surface = copy.deepcopy(SURFACE)
        surface["functions"][0]["params"] = [
            {"name": "num", "type": "things.nothing", "description": "A number."}
        ]
        docs.read(surface)
        with self.assertRaises(SystemExit) as caught:
            for module in surface["modules"]:
                docs.render_page(module, surface)
        report = str(caught.exception)
        self.assertIn("things.nothing", report)
        self.assertIn(surface["functions"][0]["path"], report)

    def test_a_reference_says_which_declaration_it_was_read_from(self):
        """The same path is written in a dozen places; only one has to move."""
        surface = copy.deepcopy(SURFACE)
        surface["functions"][1]["description"] = "See `trx.things.gone`."
        docs.read(surface)
        with self.assertRaises(SystemExit) as caught:
            for module in surface["modules"]:
                docs.render_page(module, surface)
        report = str(caught.exception)
        self.assertIn("`trx.things.gone`", report)
        self.assertIn(surface["functions"][1]["path"], report)

    def test_a_backticked_name_that_points_nowhere_is_reported(self):
        """A name in backticks reads as something to go and look at.

        It is either a path the API declares, written in full, or a literal -
        a file name, a config key, a value a setting takes - and saying which
        is what the marker is for.
        """
        self.assertEqual(docs.unreferenced(SURFACE), [])

        surface = copy.deepcopy(SURFACE)
        surface["functions"][0]["description"] = "Spawns it, as `spawn_thing` does."
        report = docs.unreferenced(surface)
        self.assertEqual(len(report), 1, report)
        self.assertIn("things.spawn: `spawn_thing` names nothing.", report[0])

        # Lua's own words and the types a declaration is written in are not
        # references, and neither is anything that is not identifier-shaped.
        for text in (
            "Comes back as `nil`, or a `table` of `integer`.",
            "Iterable with `pairs()`.",
            "Answers `-h` and `--help`, e.g. `{ key, value }` or `1`.",
            "One of `OFF` or `ON`.",
        ):
            with self.subTest(text=text):
                surface = copy.deepcopy(SURFACE)
                surface["functions"][0]["description"] = text
                self.assertEqual(docs.unreferenced(surface), [])

    def test_a_literal_is_marked_rather_than_left_loose(self):
        """The marker waives the names it lists, and only those."""
        surface = copy.deepcopy(SURFACE)
        surface["functions"][0]["description"] = (
            "Reads `visuals.water_color`, and `other_key` too. "
            "<!--noref: visuals.water_color-->"
        )
        report = docs.unreferenced(surface)
        self.assertEqual(len(report), 1, report)
        self.assertIn("`other_key` names nothing", report[0])

        # What the marker waives is the author's business, not the reader's,
        # so it comes off the page along with the space it sat in.
        surface["functions"][0]["description"] = (
            "Reads `visuals.water_color`. <!--noref: visuals.water_color--> Once."
        )
        page = rendered(surface)
        self.assertIn("Reads `visuals.water_color`. Once.", page)
        self.assertNotIn("noref", page)

    def test_a_member_that_points_somewhere_is_documented(self):
        """A `ref` says what the member is, so it is not missing prose."""
        surface = copy.deepcopy(SURFACE)
        del surface["properties"][0]["description"]
        surface["properties"][0]["type"] = "things.State"
        docs.read(surface)
        self.assertEqual(docs.undocumented(surface), [])

    def test_dump_extracts_the_json_from_a_noisy_stream(self):
        """The JSON payload is picked out of a stream that also carries logs."""
        stream = 'INF | starting\nDBG | loading\n{"modules": []}\n'
        payload = next(
            (line for line in stream.splitlines() if line.startswith("{")), None
        )
        self.assertEqual(json.loads(payload), {"modules": []})

if __name__ == "__main__":
    unittest.main(verbosity=2)
