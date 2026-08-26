# The unit tests are their own project, deliberately.
#
# They need no game data, no level, no replay and no TRX binary - and building
# them apart from the engine is what keeps that true. The engine needs SDL, the
# ffmpeg libraries, GLEW and PCRE2 to configure at all; none of that is required
# to exercise a leaf module, so none of it is here. If a test ever does need the
# engine, it will fail to configure rather than quietly grow a dependency on it.
#
# `meson test` needs only an exit code, so the harness is a header, not a
# dependency.
#
# Each test is one `unit_tests +=` stanza at the bottom of this file, naming the
# engine sources it needs and nothing more. A link error is the signal that a
# leaf module stopped being one, so the closures are listed by hand rather than
# taken from a library of everything.

project(
  'TRX unit tests',
  'c',
  default_options: ['c_std=c2x', 'warning_level=3'],
  meson_version: '>=1.3.0',
)

c_compiler = meson.get_compiler('c')
dep_mathlibrary = c_compiler.find_library('m', required: false)

# We compile engine sources, so hold them to the engine's warning policy - see
# common_build_opts in src/meson.build. Otherwise code that builds clean in the
# game warns only here.
add_project_arguments(
  ['-Wno-unused', '-Wno-unused-parameter', '-fms-extensions'],
  language: 'c',
)

# Engine sources built here answer what they measured themselves against, so a
# test can name the wording that holds a dialog to its size without piloting
# the dialog to find it.
add_project_arguments('-DTESTING', language: 'c')

# The engine takes the dependency name from -Dlua_dep; standing alone, this
# project has to find Lua itself, and distributions disagree on the name.
dep_lua = dependency('lua5.4', 'lua-5.4', 'lua54', 'lua')

# core/strings does its regex with PCRE2.
dep_pcre2 = dependency('libpcre2-8')

# The injection files the UI layout test reads its font metrics from are zlib
# compressed, the same as the game finds them, and so is the cutseq pak.
dep_zlib = dependency('zlib')

# enum_map.c hashes with uthash, which is headers only.
c_compiler.check_header('uthash.h', required: true)

# <trx/...> resolves against src/, and <harness/...> and <fakes/...> against
# src/tests. src/ comes first, so an engine header wins over a same-named file
# in the mirror below - and the mirror holds test sources only, no headers.
src_inc = include_directories('..')
test_inc = include_directories('.')
test_stubs = files('harness/stubs.c')

# The tests that read from the tree - the Lua declarations, the shipped config -
# are two levels up from this project.
repo_root = meson.current_source_dir() / '../..'

# The public Lua surface, driven the way a level script drives it. Everything
# below the assertions is real - the FIELD_DESC tables, the struct and enum
# bridges, the trxc modules, and src/lua/api/*.lua read from the tree - and only
# the engine underneath is faked. The modules are read from disk rather than from
# the embedded copy, so the tests need to know where the tree is.
b_lua_surface = {
  'engine': [
    'core/dynamic_enum.c',
    'core/enum_map.c',
    'core/handle.c',
    'core/memory.c',
    'core/result.c',
    'core/subsystem.c',
    'core/value.c',
    'core/vector.c',
    'game/enum.c',
    'game/lua/capi/api.c',
    'game/lua/capi/enum.c',
    'game/lua/capi/struct.c',
    'game/lua/field.c',
    'game/lua/registry.c',
    'game/lua/sandbox.c',
    'game/lua/utils.c',
  ],
  'src': [
    'harness/fake_calls.c',
    'harness/lua_surface.c',
    'harness/stubs_enum_map.c',
    'harness/stubs_lua_context.c',
    'harness/stubs_result.c',
  ],
  'deps': [dep_lua],
  'args': ['-DREPO_ROOT="' + repo_root + '"'],
}

# The primitive value taxonomy and the two registries it reads names out of.
# Its formatting helper and the localized game strings behind enum labels are
# stubbed: the real ones live in translation units that want PCRE2 and the game
# string table, and neither is what a value is being tested for.
b_value = {
  'engine': [
    'core/dynamic_enum.c',
    'core/enum_map.c',
    'core/memory.c',
    'core/value.c',
    'core/vector.c',
  ],
  'src': ['harness/stubs_enum_map.c'],
}

# The config layer as the game has it: the registry, the options, the change
# report. The settings file is stubbed - what the file holds is not what a
# binding is tested for - and fakes/config.c says which options exist.
b_config = {
  'engine': [
    'config/common.c',
    'config/legacy.c',
    'config/option.c',
    'config/priv.c',
    'config/registry.c',
    'config/section.c',
    'core/event_manager.c',
    'core/json/base.c',
    'core/json/parse.c',
    'version.c',
  ],
  'src': ['fakes/config.c', 'harness/stubs_config_file.c'],
}

# The string bridge, with the real fuzzy matcher and the real regex underneath
# it. PCRE2 comes with them, so a test cannot take the sources and forget the
# code unit width the headers insist on.
b_lua_strings = {
  'engine': [
    'core/strings/case_funcs.c',
    'core/strings/common.c',
    'core/strings/fuzzy_match.c',
    'game/lua/capi/strings.c',
  ],
  'deps': [dep_pcre2],
  'args': ['-DPCRE2_CODE_UNIT_WIDTH=8'],
}

# What it takes to dispatch a console command, on top of b_lua_surface: the
# console itself, the strings it prints and the log it prints them through.
b_console = {
  'engine': [
    'core/completion.c',
    'game/lua/capi/console.c',
    'game/lua/capi/locale.c',
    'game/lua/capi/log.c',
  ],
  'src': ['fakes/console.c', 'fakes/locale.c'],
}

# The event bridge, which trx.signal listens to for its tick. A module offering
# signals needs it. The item action interceptor it installs does nothing here,
# because no test that only wants the tick exercises flipeffects.
b_lua_events = {
  'engine': ['game/lua/capi/events.c'],
  'src': ['harness/stubs_item_action.c'],
}

# The trig bridge. A module that hands back a math.Box needs trx.math loaded,
# and loading it means its own declarations resolve, so the bridge comes along.
b_lua_math = {
  'engine': ['core/math/trig.c', 'game/lua/capi/math.c'],
  'deps': [dep_mathlibrary],
}

# One stanza per test. `name` is the test source, minus its .c, and mirrors what
# the test exercises: src/tests/trx/core/vector.c tests src/trx/core/vector.c,
# and src/tests/lua/api/music.c stands trx.music up out of src/lua/api/music.lua.
# The same name with dots for slashes is what `meson test` calls it.
#
# The rest says what the test needs on top of the harness:
#
#   bundles - shared source groups, unioned into the target. They do not nest,
#             so a test that wants the console names the surface as well
#   src     - further test-side sources, relative to src/tests
#   engine  - engine sources under test, relative to src/trx
#   deps    - dependencies beyond the ones a bundle brings
#   args    - c_args beyond the ones a bundle brings
#   stubs   - false to leave harness/stubs.c out
unit_tests = []

unit_tests += {
  'name': 'trx/core/vector',
  'engine': ['core/vector.c', 'core/memory.c'],
}

unit_tests += {
  'name': 'trx/core/math/func',
  'engine': ['core/math/func.c', 'core/math/geom.c', 'core/math/trig.c'],
  'deps': [dep_mathlibrary],
}

unit_tests += {
  'name': 'trx/core/math/geom',
  'engine': [
    'core/math/func.c',
    'core/math/geom.c',
    'core/math/trig.c',
    'game/matrix.c',
  ],
  'deps': [dep_mathlibrary],
}

unit_tests += {
  'name': 'trx/game/matrix',
  'engine': [
    'game/matrix.c',
    'core/math/func.c',
    'core/math/geom.c',
    'core/math/trig.c',
  ],
  'deps': [dep_mathlibrary],
}

unit_tests += {
  'name': 'trx/game/cutseq/decoder',
  'engine': ['game/cutseq/decoder.c'],
}

unit_tests += {
  'name': 'trx/game/cutseq/pak',
  'engine': [
    'game/cutseq/pak.c',
    'core/memory.c',
    'core/result.c',
    'core/subsystem.c',
  ],
  'src': ['harness/stubs_result.c'],
  'deps': [dep_zlib],
}

unit_tests += {
  'name': 'trx/core/hash',
  'engine': ['core/hash.c'],
}

unit_tests += {
  'name': 'trx/game/game_strings/lang_match',
  'engine': [
    'core/memory.c',
    'core/strings/case_funcs.c',
    'core/strings/common.c',
    'core/strings/fuzzy_match.c',
    'core/vector.c',
    'game/game_strings/lang_match.c',
  ],
  'deps': [dep_pcre2],
  'args': ['-DPCRE2_CODE_UNIT_WIDTH=8'],
}

unit_tests += {
  'name': 'trx/core/bson',
  'engine': [
    'core/bson/parse.c',
    'core/bson/write.c',
    'core/json/base.c',
    'core/memory.c',
    'core/result.c',
  ],
  'src': ['harness/stubs_result.c'],
}

unit_tests += {
  'name': 'trx/core/json',
  'engine': [
    'core/json/base.c',
    'core/json/parse.c',
    'core/json/write.c',
    'core/memory.c',
  ],
}

unit_tests += {
  'name': 'trx/core/handle',
  'engine': ['core/handle.c'],
}

# core/filesystem formats paths with core/strings, so PCRE2 comes along, and it
# stats open files through POSIX, which -std=c2x alone hides.
unit_tests += {
  'name': 'trx/core/file',
  'engine': [
    'core/result.c',
    'core/file.c',
    'core/filesystem.c',
    'core/memory.c',
    'core/strings/common.c',
    'core/vector.c',
  ],
  'src': ['harness/stubs_result.c'],
  'deps': [dep_pcre2],
  'args': ['-DPCRE2_CODE_UNIT_WIDTH=8', '-D_DEFAULT_SOURCE'],
}

unit_tests += {
  'name': 'trx/core/filesystem',
  'engine': [
    'core/result.c',
    'core/file.c',
    'core/filesystem.c',
    'core/memory.c',
    'core/strings/common.c',
    'core/vector.c',
  ],
  'src': ['harness/stubs_result.c'],
  'deps': [dep_pcre2],
  'args': ['-DPCRE2_CODE_UNIT_WIDTH=8', '-D_DEFAULT_SOURCE'],
}

unit_tests += {
  'name': 'trx/game/items/actions',
  'engine': ['core/memory.c', 'game/catalog/table.c', 'game/items/actions/common.c'],
}

unit_tests += {
  'name': 'trx/game/items/trigger',
  'engine': ['game/items/trigger.c', 'version.c'],
  'deps': [dep_mathlibrary],
}

unit_tests += {
  'name': 'trx/game/game_flow/inventory',
  'engine': ['core/memory.c', 'game/catalog/table.c', 'game/game_flow/inventory.c'],
}

unit_tests += {
  'name': 'trx/game/savegame/resume',
  'src': ['fakes/lua_store.c'],
  'engine': [
    'game/savegame/resume.c',
    'game/savegame/vars.c',
    'game/waypoint.c',
    'core/memory.c',
  ],
}

unit_tests += {
  'name': 'trx/game/console/completion',
  'engine': [
    'core/completion.c',
    'game/console/completion.c',
    'game/console/registry.c',
    'core/memory.c',
    'core/result.c',
    'core/subsystem.c',
    'core/vector.c',
  ],
  'src': ['harness/stubs_result.c'],
}

unit_tests += {
  'name': 'trx/core/completion',
  'engine': ['core/completion.c', 'core/memory.c', 'core/vector.c'],
}

unit_tests += {
  'name': 'trx/game/lua/field',
  'bundles': [b_value],
  'engine': ['game/lua/field.c'],
}

# One option: what it holds, and the stack that holds it away from that. What is
# not here is everything around it - no registry, no settings file, no game.
unit_tests += {
  'name': 'trx/config/option',
  'bundles': [b_value],
  'engine': ['config/option.c'],
}

# Writing a setting through the value it is stated as. The registry and the
# options are real; the settings file and the sections are not reached.
unit_tests += {
  'name': 'trx/config/common',
  'bundles': [b_value],
  'engine': [
    'config/common.c',
    'config/option.c',
    'config/priv.c',
    'config/registry.c',
    'config/legacy.c',
    'config/section.c',
    'core/event_manager.c',
    'core/json/base.c',
    'core/json/parse.c',
    'core/result.c',
    'core/subsystem.c',
    'version.c',
  ],
  'src': ['harness/stubs_config_file.c', 'harness/stubs_result.c'],
}

# The migrations that carry an older release's settings file forward. They
# write to g_Config and to nothing else, so the config layer above them is not
# here - no option map, no settings file.
unit_tests += {
  'name': 'trx/config/legacy',
  'engine': [
    'config/legacy.c',
    'core/json/base.c',
    'core/json/parse.c',
    'core/memory.c',
    'version.c',
  ],
}

# The name<->value registry the Lua enum bridge reflects, and the dynamic
# registry keyed on an opaque token. Both are narrower than b_value, so they
# stand up only the half they are named for.
unit_tests += {
  'name': 'trx/core/enum_map',
  'engine': ['core/enum_map.c', 'core/memory.c', 'core/vector.c'],
  'src': ['harness/stubs_enum_map.c'],
}

unit_tests += {
  'name': 'trx/core/dynamic_enum',
  'engine': ['core/dynamic_enum.c', 'core/memory.c', 'core/vector.c'],
  'src': ['harness/stubs_enum_map.c'],
}

unit_tests += {
  'name': 'trx/core/value',
  'bundles': [b_value],
}

unit_tests += {
  'name': 'trx/game/lua/capi/enum',
  'bundles': [b_value],
  'engine': [
    'game/lua/capi/enum.c',
    'game/lua/registry.c',
    'game/lua/utils.c',
  ],
  'deps': [dep_lua],
}

unit_tests += {
  'name': 'trx/game/lua/common',
  'engine': [
    'core/dynamic_enum.c',
    'core/enum_map.c',
    'core/memory.c',
    'core/result.c',
    'core/value.c',
    'core/vector.c',
    'game/lua/capi/events.c',
    'game/lua/common.c',
    'game/lua/registry.c',
    'game/lua/utils.c',
  ],
  'src': [
    'harness/stubs_console.c',
    'harness/stubs_enum_map.c',
    'harness/stubs_game_script.c',
    'harness/stubs_result.c',
  ],
  'deps': [dep_lua],
}

# The sandbox depends on nothing but Lua, so the wall the engine stands up at
# boot stands up here against a bare state - no modules, no engine.
unit_tests += {
  'name': 'trx/game/lua/sandbox',
  'engine': ['game/lua/sandbox.c'],
  'deps': [dep_lua],
}

# The guard reads nothing but Lua and the clock, and the test supplies the
# clock itself, stepped by hand, so the budget runs out deterministically.
unit_tests += {
  'name': 'trx/game/lua/guard',
  'engine': ['game/lua/guard.c'],
  'deps': [dep_lua],
}

# field.c and lua/struct.c are leaf modules, so the reflection layer and the
# Lua bridge can be exercised against a synthetic struct type.
unit_tests += {
  'name': 'trx/game/lua/capi/struct',
  'bundles': [b_value],
  'engine': [
    'core/handle.c',
    'game/lua/capi/struct.c',
    'game/lua/field.c',
    'game/lua/registry.c',
    'game/lua/utils.c',
  ],
  'deps': [dep_lua],
}

unit_tests += {
  'name': 'lua/api/rooms',
  'bundles': [b_lua_surface, b_lua_math],
  'src': ['harness/stubs_console.c', 'fakes/rooms.c', 'fakes/items.c'],
  'engine': [
    'game/lua/capi/events.c',
    'game/lua/capi/items.c',
    'game/lua/capi/rooms.c',
  ],
}

unit_tests += {
  'name': 'lua/api/items',
  'bundles': [b_lua_surface, b_lua_strings, b_lua_math],
  'src': ['harness/stubs_console.c', 'fakes/items.c', 'fakes/catalog.c'],
  'engine': [
    'game/lua/capi/catalog.c',
    'game/lua/capi/events.c',
    'game/lua/capi/items.c',
    'game/lua/capi/objects.c',
  ],
}

# The same surface, taken through the two boot steps the other suites stop
# short of: the seal, and hardening the globals.
unit_tests += {
  'name': 'lua/boot',
  'bundles': [b_lua_surface, b_lua_math],
  'src': ['fakes/items.c'],
  'engine': ['game/lua/capi/items.c'],
}

# No fake engine: the real trig tables are the thing under test.
unit_tests += {
  'name': 'lua/api/math',
  'bundles': [b_lua_surface, b_lua_math],
}

# Signals: what they hold and when they say so. The config layer stands here
# because a setting speaks through a signal of its own, and that is what it
# speaks with.
unit_tests += {
  'name': 'lua/api/signal',
  'bundles': [b_lua_surface, b_config, b_lua_math, b_lua_strings],
  'src': ['fakes/level.c', 'harness/stubs_console.c', 'fakes/locale.c', 'fakes/rooms.c'],
  'engine': [
    'game/lua/capi/config.c',
    'game/lua/capi/events.c',
    'game/lua/capi/locale.c',
    'game/ui/dialogs/settings_handlers.c',
    'game/ui/dialogs/settings_lua.c',
    'game/ui/dialogs/settings_rows.c',
  ],
}

# No fake engine either: the real generator is the thing under test. trx.math
# comes along for the angle unit that trx.random.angle reports in.
unit_tests += {
  'name': 'lua/api/random',
  'bundles': [b_lua_surface, b_lua_math],
  'engine': [
    'core/result.c',
    'core/subsystem.c',
    'game/lua/capi/random.c',
    'game/random.c',
  ],
  'src': ['harness/stubs_result.c'],
}

unit_tests += {
  'name': 'lua/api/store',
  'bundles': [b_lua_surface, b_lua_strings],
  'src': ['lua/api/store.c'],
  'engine': [
    'core/colors.c',
    'core/json/base.c',
    'core/json/util/read_io.c',
    'core/json/util/value.c',
    'core/json/util/write_io.c',
    'game/lua/capi/store.c',
  ],
  'deps': [dep_mathlibrary],
}

unit_tests += {
  'name': 'lua/api/ui',
  'bundles': [b_lua_surface, b_lua_math, b_config],
  'src': [
    'fakes/level.c',
    'lua/api/ui.c',
    'fakes/objects.c',
    'fakes/sprites.c',
    'fakes/ui_draw.c',
    'fakes/locale.c',
    'fakes/rooms.c',
  ],
  'engine': [
    'config/enum.c',
    'core/event_manager.c',
    'game/ui/events.c',
    'game/lua/capi/config.c',
    'game/lua/capi/events.c',
    'game/lua/capi/locale.c',
    'game/lua/capi/ui.c',
    'game/ui/dialogs/settings_handlers.c',
    'game/ui/dialogs/settings_lua.c',
    'game/ui/dialogs/settings_rows.c',
    'game/ui/common.c',
    'game/ui/regions.c',
    'game/ui/elements/anchor.c',
    'game/ui/elements/frame.c',
    'game/ui/elements/hide.c',
    'game/ui/elements/offset.c',
    'game/ui/elements/pad.c',
    'game/ui/elements/resize.c',
    'game/ui/elements/spacer.c',
    'game/ui/elements/span.c',
    'game/ui/elements/stack.c',
    'game/ui/helpers.c',
    'game/ui/scaler.c',
  ],
}

unit_tests += {
  'name': 'lua/api/assault',
  'bundles': [b_lua_surface],
  'src': ['fakes/assault.c'],
  'engine': ['game/lua/capi/assault.c'],
}

# rooms rides along: trx.camera.room hands back a trx.rooms.Room.
unit_tests += {
  'name': 'lua/api/camera',
  'bundles': [b_lua_surface, b_lua_math],
  'src': [
    'fakes/camera.c',
    'fakes/rooms.c',
    # for Room_GetIndexFromPos, which rooms.c reaches for
    'fakes/items.c',
  ],
  'engine': [
    'game/lua/capi/camera.c',
    'game/lua/capi/rooms.c',
  ],
}

unit_tests += {
  'name': 'lua/api/music',
  'bundles': [b_lua_surface],
  'src': ['fakes/music.c'],
  'engine': ['game/lua/capi/music.c'],
}

unit_tests += {
  'name': 'lua/api/strings',
  'bundles': [b_lua_surface, b_lua_strings],
}

# Pure Lua, with no bridge under it: the encoder is the whole module.
unit_tests += {
  'name': 'lua/api/json',
  'bundles': [b_lua_surface],
}

# The parser is pure Lua, but it leans on the real fuzzy matcher out of
# trx.strings, so the two are stood up together.
unit_tests += {
  'name': 'lua/api/argparse',
  'bundles': [b_lua_surface, b_lua_strings],
  'src': ['fakes/locale.c'],
  'engine': ['game/lua/capi/locale.c'],
}

unit_tests += {
  'name': 'lua/api/sound',
  'bundles': [b_lua_surface],
  'src': ['fakes/sound.c'],
  'engine': ['game/lua/capi/sound.c'],
}

unit_tests += {
  'name': 'lua/api/stats',
  'bundles': [b_lua_surface, b_lua_events],
  'src': ['harness/stubs_console.c', 'fakes/game.c', 'fakes/stats.c'],
  'engine': ['game/lua/capi/game.c', 'game/lua/capi/stats.c'],
}

unit_tests += {
  'name': 'lua/api/locale',
  'bundles': [b_lua_surface],
  'src': ['fakes/locale.c'],
  'engine': ['game/lua/capi/locale.c'],
}

unit_tests += {
  'name': 'lua/api/lua',
  'bundles': [b_lua_surface],
  'src': ['fakes/lua.c'],
  'engine': ['game/lua/capi/lua.c'],
}

# The engine under the bridge is the real spatial query and the real event
# registry, so the suite says whether the zones work rather than whether the
# module repeats what we meant.
unit_tests += {
  'name': 'lua/api/zones',
  'bundles': [b_lua_surface, b_lua_math],
  'src': ['fakes/level.c',
    'harness/stubs_console.c',
    'fakes/items.c',
    'fakes/rooms.c',
    'fakes/lara.c',
    'fakes/catalog.c',
    'fakes/camera.c',
  ],
  'engine': [
    'game/lua/capi/camera.c',
    'game/lua/capi/catalog.c',
    'game/lua/capi/events.c',
    'game/lua/capi/items.c',
    'game/lua/capi/lara.c',
    'game/lua/capi/rooms.c',
  ],
}

unit_tests += {
  'name': 'lua/api/weather',
  'bundles': [b_lua_surface],
  'src': ['fakes/weather.c'],
  'engine': ['game/lua/capi/weather.c'],
}

unit_tests += {
  'name': 'lua/api/mod',
  'bundles': [b_lua_surface],
  'engine': ['game/lua/capi/mod.c'],
}

unit_tests += {
  'name': 'lua/api/savegame',
  'bundles': [b_lua_surface],
  'engine': ['game/lua/capi/savegame.c'],
}

# No stubs: fakes/log.c provides Log_Message itself, because which line
# the log blames is the thing under test.
unit_tests += {
  'name': 'lua/api/log',
  'bundles': [b_lua_surface],
  'src': ['fakes/log.c'],
  'engine': ['game/lua/capi/log.c'],
  'stubs': false,
}

unit_tests += {
  'name': 'lua/api/game',
  'bundles': [b_lua_surface, b_lua_events],
  'src': ['harness/stubs_console.c', 'fakes/game.c'],
  'engine': ['game/lua/capi/game.c'],
}

unit_tests += {
  'name': 'lua/api/objects',
  'bundles': [b_lua_surface, b_lua_strings],
  'src': ['fakes/items.c', 'fakes/catalog.c'],
  'engine': ['game/lua/capi/catalog.c', 'game/lua/capi/objects.c'],
}

# The catalog rides along: the weapons a script names are its enum.
unit_tests += {
  'name': 'lua/api/inventory',
  'bundles': [b_lua_surface, b_lua_math],
  'src': ['fakes/game.c', 'fakes/lara.c', 'fakes/items.c', 'fakes/catalog.c'],
  'engine': [
    'game/lua/capi/catalog.c',
    'game/lua/capi/inventory.c',
    'game/lua/capi/items.c',
    'game/lua/capi/weapons.c',
  ],
}

# The weapon table is the engine's own; the fake stands in for the gun code the
# bridges ask which pickup a weapon is and what it is drawn from.
unit_tests += {
  'name': 'lua/api/weapons',
  'bundles': [b_lua_surface, b_lua_math],
  'src': ['fakes/lara.c', 'fakes/items.c', 'fakes/catalog.c'],
  'engine': [
    'game/lua/capi/catalog.c',
    'game/lua/capi/items.c',
    'game/lua/capi/weapons.c',
  ],
}

unit_tests += {
  'name': 'lua/api/lara',
  'bundles': [b_lua_surface, b_lua_math, b_lua_events],
  'src': [
    'harness/stubs_console.c',
    'fakes/level.c',
    'fakes/lara.c',
    'fakes/items.c',
    'fakes/catalog.c',
  ],
  'engine': [
    'game/lua/capi/catalog.c',
    'game/lua/capi/items.c',
    'game/lua/capi/lara.c',
  ],
}

# The catalogs are real ENUM_MAP enums out of the real trx/game/enum.c. Only
# the slot mapping, which a game loads from a CSV, is faked.
unit_tests += {
  'name': 'lua/api/catalog',
  'bundles': [b_lua_surface],
  'src': ['fakes/catalog.c'],
  'engine': ['game/lua/capi/catalog.c'],
}

unit_tests += {
  'name': 'lua/api/config',
  'bundles': [b_lua_surface, b_config, b_lua_math, b_lua_strings],
  'src': ['fakes/level.c', 'harness/stubs_console.c', 'fakes/locale.c'],
  'engine': [
    'game/lua/capi/config.c',
    'game/lua/capi/locale.c',
    'game/ui/dialogs/settings_handlers.c',
    'game/ui/dialogs/settings_lua.c',
    'game/ui/dialogs/settings_rows.c',
  ],
}

unit_tests += {
  'name': 'lua/api/console',
  'bundles': [b_lua_surface, b_console, b_lua_strings],
}

# The help command, which the console module registers and composes from the
# same parser a command answers --help from. It needs nothing the console
# itself does not.
unit_tests += {
  'name': 'lua/commands/help',
  'bundles': [b_lua_surface, b_console, b_lua_strings],
}

# A console command written in Lua, exercised through the console that
# dispatches it. Everything the command touches is a real bridge, so the test
# says whether the command works, not whether it was written the way we
# expected.
# The version command, exercised through the console that dispatches it. It
# reads what the build calls itself and logs it, so the test watches the log.
unit_tests += {
  'name': 'lua/commands/version',
  'bundles': [b_lua_surface, b_console, b_lua_events],
  'src': ['fakes/game.c'],
  'engine': ['game/lua/capi/game.c'],
}

unit_tests += {
  'name': 'lua/commands/heal',
  'bundles': [b_lua_surface, b_console, b_lua_math, b_lua_events],
  'src': ['fakes/level.c', 'fakes/game.c', 'fakes/items.c', 'fakes/lara.c'],
  'engine': [
    'game/lua/capi/game.c',
    'game/lua/capi/items.c',
    'game/lua/capi/lara.c',
  ],
}

# The commands with non-trivial arguments, exercised through the console. Name
# matching goes through fakes/strings.c, a pcre2-free stand-in, so these
# stay light: the real matcher is covered by lua_strings and lua_argparse.
unit_tests += {
  'name': 'lua/commands/play_level',
  'bundles': [b_lua_surface, b_console, b_lua_events],
  'src': ['fakes/game.c', 'fakes/strings.c'],
  'engine': ['game/lua/capi/game.c'],
}

unit_tests += {
  'name': 'lua/commands/music',
  'bundles': [b_lua_surface, b_console],
  'src': ['fakes/music.c', 'fakes/strings.c'],
  'engine': ['game/lua/capi/music.c'],
}

unit_tests += {
  'name': 'lua/commands/load_game',
  'bundles': [b_lua_surface, b_console],
  'src': ['fakes/savegame.c'],
}

unit_tests += {
  'name': 'lua/commands/rule',
  'bundles': [b_lua_surface, b_console, b_lua_strings],
  'engine': ['game/lua/capi/rules.c', 'game/rules.c'],
}

unit_tests += {
  'name': 'lua/commands/secret',
  'bundles': [b_lua_surface, b_console, b_lua_strings, b_lua_events],
  'src': ['fakes/game.c', 'fakes/stats.c'],
  'engine': ['game/lua/capi/game.c', 'game/lua/capi/stats.c'],
}

unit_tests += {
  'name': 'lua/commands/give',
  'bundles': [
    b_lua_surface, b_console, b_lua_math, b_lua_strings, b_lua_events,
  ],
  'src': [
    'fakes/level.c',
    'fakes/game.c',
    'fakes/items.c',
    'fakes/lara.c',
    'fakes/catalog.c',
    'fakes/sound.c',
  ],
  'engine': [
    'game/lua/capi/catalog.c',
    'game/lua/capi/game.c',
    'game/lua/capi/inventory.c',
    'game/lua/capi/items.c',
    'game/lua/capi/lara.c',
    'game/lua/capi/objects.c',
    'game/lua/capi/sound.c',
    'game/lua/capi/weapons.c',
  ],
}

unit_tests += {
  'name': 'modules/overlay',
  'bundles': [b_lua_surface, b_lua_math, b_lua_strings],
  'src': [
    'harness/stubs_config_file.c',
    'harness/stubs_console.c',
    'fakes/camera.c',
    'fakes/catalog.c',
    'fakes/creatures.c',
    'fakes/cutseq.c',
    'fakes/game.c',
    'fakes/items.c',
    'fakes/lara.c',
    'fakes/level.c',
    'fakes/locale.c',
    'fakes/overlay.c',
    'fakes/rooms.c',
    'fakes/sprites.c',
    'fakes/ui_draw.c',
  ],
  'engine': [
    'config/common.c',
    'config/enum.c',
    'config/legacy.c',
    'config/map.c',
    'config/option.c',
    'config/priv.c',
    'config/option_text.c',
    'config/registry.c',
    'config/section.c',
    'core/colors.c',
    'core/event_manager.c',
    'core/file.c',
    'core/filesystem.c',
    'core/json/base.c',
    'core/json/parse.c',
    'core/json/util/file.c',
    'core/json/util/read_io.c',
    'core/json/write.c',
    'game/lua/capi/camera.c',
    'game/lua/capi/catalog.c',
    'game/lua/capi/config.c',
    'game/lua/capi/cutscenes.c',
    'game/lua/capi/events.c',
    'game/lua/capi/game.c',
    'game/lua/capi/inventory.c',
    'game/lua/capi/items.c',
    'game/lua/capi/lara.c',
    'game/lua/capi/locale.c',
    'game/lua/capi/objects.c',
    'game/lua/capi/overlay.c',
    'game/lua/capi/rooms.c',
    'game/lua/capi/rules.c',
    'game/lua/capi/ui.c',
    'game/lua/capi/weapons.c',
    'game/lua/ui.c',
    'game/rules.c',
    'game/ui/common.c',
    'game/ui/elements/anchor.c',
    'game/ui/elements/frame.c',
    'game/ui/elements/hide.c',
    'game/ui/elements/offset.c',
    'game/ui/elements/pad.c',
    'game/ui/elements/resize.c',
    'game/ui/elements/row_arrows.c',
    'game/ui/elements/spacer.c',
    'game/ui/elements/span.c',
    'game/ui/elements/stack.c',
    'game/ui/events.c',
    'game/ui/helpers.c',
    'game/ui/regions.c',
    'game/ui/scaler.c',
  ],
  'args': ['-D_DEFAULT_SOURCE'],
  'stubs': false,
}

unit_tests += {
  'name': 'lua/commands/teleport',
  'bundles': [b_lua_surface, b_console, b_lua_strings, b_lua_math],
  'src': ['fakes/level.c',
    'fakes/camera.c',
    'fakes/catalog.c',
    'fakes/game.c',
    'fakes/items.c',
    'fakes/lara.c',
    'fakes/rooms.c',
  ],
  'engine': [
    'game/lua/capi/camera.c',
    'game/lua/capi/catalog.c',
    'game/lua/capi/events.c',
    'game/lua/capi/game.c',
    'game/lua/capi/items.c',
    'game/lua/capi/lara.c',
    'game/lua/capi/objects.c',
    'game/lua/capi/rooms.c',
    'core/math/trig.c',
  ],
  'deps': [dep_mathlibrary],
}

unit_tests += {
  'name': 'lua/commands/save_game',
  'bundles': [b_lua_surface, b_console, b_lua_events],
  'src': ['fakes/game.c', 'fakes/savegame.c'],
  'engine': ['game/lua/capi/game.c'],
}

unit_tests += {
  'name': 'lua/commands/weather',
  'bundles': [b_lua_surface, b_console, b_lua_events],
  'src': ['fakes/game.c', 'fakes/strings.c', 'fakes/weather.c'],
  'engine': ['game/lua/capi/game.c', 'game/lua/capi/weather.c'],
}

unit_tests += {
  'name': 'lua/commands/set',

  'src': ['fakes/level.c'],  'bundles': [b_lua_surface, b_config, b_lua_math, b_lua_strings, b_console],
  'engine': ['game/lua/capi/config.c'],
}

unit_tests += {
  'name': 'lua/api/creatures',
  'bundles': [b_lua_surface],
  'src': ['fakes/creatures.c'],
  'engine': ['game/lua/capi/creatures.c'],
}

unit_tests += {
  'name': 'lua/api/events',
  'bundles': [b_lua_surface],
  'src': ['fakes/rooms.c'],
  'engine': ['game/lua/capi/events.c'],
}

unit_tests += {
  'name': 'lua/seal',
  'bundles': [b_lua_surface, b_lua_math],
  'src': ['fakes/items.c'],
  'engine': ['game/lua/capi/items.c'],
}

# The text a binding reads as. It knows nothing of SDL, so no backend comes
# along.
unit_tests += {
  'name': 'trx/game/input/names',
  'engine': ['game/input/names.c'],
}

unit_tests += {
  'name': 'trx/game/ui/scrollable',
  'engine': ['game/ui/scrollable.c'],
}

# Word wrapping, over the shipped font.bin of a single game: what the wrapper
# decides rests on glyph widths, so a stub font would agree with anything.
unit_tests += {
  'name': 'trx/game/ui/text',
  'src': [
    'fakes/ui.c',
    'fakes/ui_draw.c',
    'harness/font_bin.c',
    'harness/stubs_result.c',
  ],
  'engine': [
    'core/enum_map.c',
    'core/memory.c',
    'core/strings/case_funcs.c',
    'core/strings/common.c',
    'core/result.c',
    'core/subsystem.c',
    'core/vector.c',
    'game/enum.c',
    'game/game_strings/entries.c',
    'game/objects/vars.c',
    'game/ui/scaler.c',
    'game/ui/text.c',
    'version.c',
  ],
  'deps': [dep_pcre2, dep_zlib],
  'args': [
    '-DPCRE2_CODE_UNIT_WIDTH=8',
    '-DTEST_SHIP_GAMES_DIR="' + repo_root / 'data/trx/ship/games' + '"',
  ],
}

# Where a settings row sits: the tab lists and the placement a declared row
# resolves to. The options are real; nothing draws.
unit_tests += {
  'name': 'trx/game/ui/dialogs/settings_rows',
  'bundles': [b_value],
  'engine': [
    'config/common.c',
    'config/legacy.c',
    'config/option.c',
    'config/priv.c',
    'config/registry.c',
    'config/section.c',
    'core/event_manager.c',
    'core/json/base.c',
    'core/json/parse.c',
    'core/result.c',
    'core/subsystem.c',
    'game/ui/dialogs/settings_handlers.c',
    'game/ui/dialogs/settings_rows.c',
    'version.c',
  ],
  'src': ['harness/stubs_config_file.c', 'harness/stubs_result.c'],
}

# The UI layout test is the widest of the unit tests: it stands up the settings
# dialogs and runs the real measure and layout passes over them. It reads the
# shipped font.bin for glyph metrics and the shipped config for the strings -
# both versioned in this repository - so it still needs no window and no build
# of the game.
unit_tests += {
  'name': 'trx/game/ui',
  'src': [
    'harness/stubs_result.c',
    'fakes/settings.c',
    'fakes/ui.c',
    'fakes/ui_draw.c',
    'harness/font_bin.c',
  ],
  'engine': [
    'config/common.c',
    'config/enum.c',
    'config/map.c',
    'config/option.c',
    'config/option_text.c',
    'config/presets.c',
    'config/registry.c',
    'config/section.c',
    'core/colors.c',
    'core/completion.c',
    'core/dynamic_enum.c',
    'core/enum_map.c',
    'core/event_manager.c',
    'core/handle.c',
    'core/json/base.c',
    'core/json/parse.c',
    'core/json/util/file.c',
    'core/json/util/read_io.c',
    'core/json/write.c',
    'core/memory.c',
    'core/result.c',
    'core/strings/case_funcs.c',
    'core/strings/common.c',
    'core/strings/fuzzy_match.c',
    'core/subsystem.c',
    'core/value.c',
    'core/vector.c',
    'game/catalog/table.c',
    'game/enum.c',
    'game/game_strings/entries.c',
    'game/gun/registry.c',
    'game/objects/names.c',
    'game/objects/vars.c',
    'game/rules.c',
    'game/ui/common.c',
    'game/ui/regions.c',
    'game/ui/dialogs/color_editor.c',
    'game/ui/dialogs/config_presets.c',
    'game/ui/dialogs/controls_editor.c',
    'game/ui/dialogs/gameplay_settings.c',
    'game/ui/dialogs/graphic_settings.c',
    'game/ui/dialogs/setting_helpers/handlers.c',
    'game/ui/dialogs/setting_helpers/handlers_language.c',
    'game/ui/dialogs/settings.c',
    'game/ui/dialogs/settings_editor.c',
    'game/ui/dialogs/settings_handlers.c',
    'game/ui/dialogs/settings_rows.c',
    'game/ui/dialogs/settings_tabs.c',
    'game/ui/dialogs/sound_settings.c',
    'game/ui/dialogs/text.c',
    'game/ui/events.c',
    'game/ui/helpers.c',
    'game/ui/scaler.c',
    'game/ui/scaler.c',
    'game/ui/scrollable.c',
    'game/ui/settings.c',
    'game/ui/text.c',
    'version.c',
    'gl/enum.c',
    'game/ui/elements/anchor.c',
    'game/ui/elements/bar.c',
    'game/ui/elements/color_swatch.c',
    'game/ui/elements/flash.c',
    'game/ui/elements/frame.c',
    'game/ui/elements/gradient_slider.c',
    'game/ui/elements/hide.c',
    'game/ui/elements/horizontal_line.c',
    'game/ui/elements/label.c',
    'game/ui/elements/modal.c',
    'game/ui/elements/offset.c',
    'game/ui/elements/pad.c',
    'game/ui/elements/progress_button.c',
    'game/ui/elements/prompt.c',
    'game/ui/elements/requester.c',
    'game/ui/elements/resize.c',
    'game/ui/elements/row_arrows.c',
    'game/ui/elements/scrollable_stack.c',
    'game/ui/elements/sleek_bar.c',
    'game/ui/elements/spacer.c',
    'game/ui/elements/span.c',
    'game/ui/elements/stack.c',
    'game/ui/helpers.c',
    'game/ui/scaler.c',
    'game/ui/elements/tab_switch.c',
    'game/ui/elements/window.c',
  ],
  'deps': [dep_mathlibrary, dep_pcre2, dep_zlib],
  'args': [
    '-DPCRE2_CODE_UNIT_WIDTH=8',
    '-DTEST_SHIP_CFG_DIR="' + repo_root / 'data/trx/ship/cfg' + '"',
    '-DTEST_SHIP_GAMES_DIR="' + repo_root / 'data/trx/ship/games' + '"',
  ],
}

foreach spec : unit_tests
  name = spec['name']
  test_src = spec.get('src', [])
  test_engine = []
  test_deps = spec.get('deps', [])
  test_args = spec.get('args', [])

  foreach bundle : spec.get('bundles', [])
    test_src += bundle.get('src', [])
    test_engine += bundle.get('engine', [])
    test_deps += bundle.get('deps', [])
    test_args += bundle.get('args', [])
  endforeach
  test_engine += spec.get('engine', [])

  sources = files(name + '.c')
  if spec.get('stubs', true)
    sources += test_stubs
  endif
# Bundles are listed side by side rather than nested, so two of them may name
# the same source. Take it once: a target that lists one bundle too many
# should be no harder to fix than deleting the bundle again.
  seen = []
  foreach source : test_src
    if source not in seen
      seen += source
      sources += files(source)
    endif
  endforeach
  seen = []
  foreach source : test_engine
    if source not in seen
      seen += source
      sources += files('..' / 'trx' / source)
    endif
  endforeach

  test_id = '.'.join(name.split('/'))
  exe = executable(
    test_id,
    sources,
    include_directories: [src_inc, test_inc],
    dependencies: test_deps,
    c_args: test_args,
    build_by_default: false,
  )
  test(test_id, exe, suite: 'unit')
endforeach

# src/lua/api/api.lua under a plain interpreter, with the C bridge stubbed in
# Lua. It is the one suite with no runner to build.
lua_exe = find_program('lua', 'lua5.4', 'lua5.5', required: false)
if lua_exe.found()
  test(
    'lua.api.api',
    lua_exe,
    args: [files('lua/api/api.lua'), repo_root],
    suite: 'unit',
  )
endif

# The checks over tools/ and over src/meson.build itself. They exercise no
# engine code, so they mirror nothing under trx/.
python_exe = find_program('python3', required: false)
if python_exe.found()
  foreach name : ['lua_sources', 'update_lua_docs', 'game_strings', 'cfuncs', 'cdecls']
    test(
      'tools.' + name,
      python_exe,
      args: [files('tools' / name + '.py')],
      suite: 'unit',
    )
  endforeach
endif
