#!/usr/bin/env python3
"""Unit tests for tools/lint/checks/lua_number_names. No engine, no binary."""

from __future__ import annotations

import unittest

from helper import load

check = load("lint/checks/lua_number_names")


def one(spec, group="functions"):
    """A dump holding a single declaration, so a rule is read on its own."""
    if group == "functions":
        return {"functions": [{"path": "things.poke", "params": [spec]}]}
    if group == "types":
        return {"types": [{"path": "things.Widget", "fields": [spec]}]}
    if group == "containers":
        return {"containers": [{"module": "things", "key": spec}]}
    return {"properties": [dict(spec, path="things." + spec["name"])]}


def warnings(api):
    return list(check.warnings_for(api))


class NumberNames(unittest.TestCase):
    def test_num_says_where_it_counts_from(self):
        self.assertEqual(warnings(one({"name": "room_num", "base": 0})), [])

    def test_num_without_a_base_is_reported(self):
        [message] = warnings(one({"name": "room_num"}))
        self.assertIn("where it counts from", message)

    def test_bare_num_needs_a_base_too(self):
        [message] = warnings(one({"name": "num"}))
        self.assertIn("where it counts from", message)

    def test_an_index_is_reported(self):
        for name in ("index", "idx", "slot_index", "mesh_idx"):
            with self.subTest(name=name):
                [message] = warnings(one({"name": name}))
                self.assertIn("is an index", message)

    def test_an_id_carries_no_base(self):
        self.assertEqual(warnings(one({"name": "object_id"})), [])
        [message] = warnings(one({"name": "object_id", "base": 0}))
        self.assertIn("counts from nothing", message)

    def test_a_base_is_0_or_1(self):
        [message] = warnings(one({"name": "room_num", "base": 2}))
        self.assertIn("not 0 or 1", message)

    def test_every_group_is_read(self):
        for group in ("functions", "types", "containers", "properties"):
            with self.subTest(group=group):
                [message] = warnings(one({"name": "room_num"}, group))
                self.assertIn("room_num", message)

    def test_a_callbacks_own_arguments_are_read(self):
        api = {
            "functions": [
                {
                    "path": "things.on_poke",
                    "params": [
                        {
                            "name": "callback",
                            "type": "function",
                            "params": [{"name": "item_num"}],
                        }
                    ],
                }
            ]
        }
        [message] = warnings(api)
        self.assertIn("item_num", message)

    def test_a_container_key_with_no_name_reads_the_module(self):
        api = {"containers": [{"module": "rooms", "key": {"base": 0}}]}
        self.assertEqual(warnings(api), [])

    def test_a_return_is_read(self):
        api = {
            "functions": [
                {"path": "rooms.find", "returns": [{"name": "room_num"}]}
            ]
        }
        [message] = warnings(api)
        self.assertIn("room_num", message)


if __name__ == "__main__":
    unittest.main()
