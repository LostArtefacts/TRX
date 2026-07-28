#!/usr/bin/env python3
"""Unit tests for tools/lint/gen/game_strings. No engine, no binary, no game data."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from tools_helper import load


class TestGameStrings(unittest.TestCase):
    """A game string named only from Lua must not be pruned.

    The scanner reads .c/.h/.def. A key a Lua script names and nothing else does
    would count as unused, and pruning it strips the text a console command puts
    on the screen - silently, and only at runtime.
    """

    def setUp(self):
        self.strings = load("lint/gen/game_strings")

    def scan_lines(self, source: str) -> list[tuple[int, str]]:
        with tempfile.NamedTemporaryFile("w", suffix=".lua", delete=False) as fh:
            fh.write(source)
            path = Path(fh.name)
        try:
            return list(self.strings.get_used_lua_strings(path))
        finally:
            path.unlink()

    def scan(self, source: str) -> set[str]:
        return {key for _, key in self.scan_lines(source)}

    def test_lua_string_usages_are_found(self):
        found = self.scan(
            'trx.locale.get("test/plain")\ntrx.locale.format("test/formatted", n)\n'
        )
        self.assertEqual(found, {"test/plain", "test/formatted"})

    def test_command_help_id_is_found(self):
        found = self.scan('trx.console.register({ help = "test/help" })\n')
        self.assertEqual(found, {"test/help"})

    def test_single_quoted_key_is_found(self):
        # Both quoting styles are ordinary Lua, and a script that picks the one
        # the scanner cannot see loses its key to the pruning step.
        found = self.scan("trx.locale.get('test/plain')\n")
        self.assertEqual(found, {"test/plain"})

    def test_escapes_in_a_key_are_resolved(self):
        found = self.scan(r'trx.locale.get("test/say_\"hi\"")' + "\n")
        self.assertEqual(found, {'test/say_"hi"'})

    def test_unrelated_lua_strings_are_not_reported(self):
        found = self.scan('local x = "not a game string"\ntrx.log.info("hello")\n')
        self.assertEqual(found, set())

    def test_commented_out_usage_is_ignored(self):
        found = self.scan('-- trx.locale.get("test/dead_key")\n')
        self.assertEqual(found, set())

    def test_block_commented_out_usage_is_ignored(self):
        found = self.scan('--[[\ntrx.locale.get("test/dead_key")\n]]\n')
        self.assertEqual(found, set())

    def test_usage_in_a_long_string_is_ignored(self):
        # This is how the api.define examples are written. An example names a key
        # to show what a call looks like; it does not keep that key alive.
        found = self.scan('examples = { [[trx.locale.get("test/dead_key")]] },\n')
        self.assertEqual(found, set())

    def test_dashes_inside_a_string_do_not_hide_a_usage(self):
        # A naive `re.sub("--.*", "")` truncates this line at the "--" inside the
        # string literal, dropping the real usage after it and pruning a live
        # key. The usage must still be found.
        found = self.scan('local label = "a--b"; trx.locale.get("test/plain")\n')
        self.assertEqual(found, {"test/plain"})

    def test_line_numbers_survive_the_stripping(self):
        # The reported line is what a maintainer goes to when a key needs
        # chasing down, so blanking a comment must not swallow its newlines.
        found = self.scan_lines(
            '--[[\n\n]]\nlocal x = 1\ntrx.locale.get("test/plain")\n'
        )
        self.assertEqual(found, [(5, "test/plain")])


class TestLuaDeclarations(unittest.TestCase):
    """trx.locale.declare() is where a Lua script's own text lives.

    The scanner is what carries it into cfg/base_strings.json5, so a declaration
    it cannot read is a string the game ships without and the translators never
    see.
    """

    def setUp(self):
        self.strings = load("lint/gen/game_strings")

    def scan_lines(self, source: str) -> list[tuple[int, str, str]]:
        with tempfile.NamedTemporaryFile("w", suffix=".lua", delete=False) as fh:
            fh.write(source)
            path = Path(fh.name)
        try:
            return list(self.strings.get_declared_lua_strings(path))
        finally:
            path.unlink()

    def scan(self, source: str) -> dict[str, str]:
        return {key: value for _, key, value in self.scan_lines(source)}

    def test_a_declared_key_carries_its_text(self):
        found = self.scan(
            'trx.locale.declare({\n  ["test/plain"] = "Plain text",\n})\n'
        )
        self.assertEqual(found, {"test/plain": "Plain text"})

    def test_single_quoted_entries_are_found(self):
        found = self.scan("trx.locale.declare({ ['test/plain'] = 'Plain' })\n")
        self.assertEqual(found, {"test/plain": "Plain"})

    def test_escapes_are_resolved(self):
        found = self.scan(
            r'trx.locale.declare({ ["test/two"] = "a\nb\"c\\d" })' + "\n"
        )
        self.assertEqual(found, {"test/two": 'a\nb"c\\d'})

    def test_a_bracket_in_the_text_does_not_end_the_call(self):
        # "Valid values: [integer]" is real text a command prints, and a scanner
        # that balanced brackets rather than parentheses would stop inside it.
        found = self.scan(
            'trx.locale.declare({\n'
            '  ["test/a"] = "one) [two]",\n'
            '  ["test/b"] = "three",\n'
            "})\n"
        )
        self.assertEqual(found, {"test/a": "one) [two]", "test/b": "three"})

    def test_every_call_in_a_file_is_read(self):
        found = self.scan(
            'trx.locale.declare({ ["test/a"] = "A" })\n'
            'trx.locale.declare({ ["test/b"] = "B" })\n'
        )
        self.assertEqual(found, {"test/a": "A", "test/b": "B"})

    def test_a_table_that_is_not_a_declaration_is_ignored(self):
        found = self.scan('local labels = { ["test/plain"] = "Plain" }\n')
        self.assertEqual(found, {})

    def test_commented_out_declaration_is_ignored(self):
        found = self.scan(
            '-- trx.locale.declare({ ["test/dead"] = "Dead" })\n'
        )
        self.assertEqual(found, {})

    def test_declaration_in_a_long_string_is_ignored(self):
        # The api.define example shows what a call looks like; it declares
        # nothing.
        found = self.scan(
            'examples = { [[trx.locale.declare({ ["test/dead"] = "Dead" })]] },\n'
        )
        self.assertEqual(found, {})

    def test_the_reported_line_is_the_entry_s_own(self):
        found = self.scan_lines(
            "local x = 1\n"
            "trx.locale.declare({\n"
            '  ["test/a"] = "A",\n'
            '  ["test/b"] = "B",\n'
            "})\n"
        )
        self.assertEqual(found, [(3, "test/a", "A"), (4, "test/b", "B")])


if __name__ == "__main__":
    unittest.main(verbosity=2)
