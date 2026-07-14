#!/usr/bin/env python3
"""Unit tests for tools/update_game_strings. No engine, no binary, no game data."""

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
        self.strings = load("update_game_strings")

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


if __name__ == "__main__":
    unittest.main(verbosity=2)
