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
# Each test is one entry in `unit_tests` at the bottom of this file, naming the
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

# <trx/...> resolves against src/; the harness header against src/tests/unit/.
src_inc = include_directories('..')
test_inc = include_directories('unit')
test_stubs = files('unit/stubs.c')

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
  'src': ['lua_surface.c', 'stubs_enum_map.c'],
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
  'src': ['stubs_enum_map.c'],
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
  'src': ['fake_engine_console.c', 'fake_engine_locale.c'],
}

# One entry per test. The key names both the test and unit/test_<key>.c; the
# value says what that test needs on top of the harness:
#
#   bundles - shared source groups, unioned into the target. They do not nest,
#             so a test that wants the console names the surface as well
#   src     - further test-side sources, relative to unit/
#   engine  - engine sources under test, relative to ../trx/
#   deps    - dependencies beyond the ones a bundle brings
#   args    - c_args beyond the ones a bundle brings
#   stubs   - false to leave unit/stubs.c out
unit_tests = {
  'vector': {'engine': ['core/vector.c', 'core/memory.c']},
  'math_func': {
    'engine': ['core/math/func.c', 'core/math/geom.c', 'core/math/trig.c'],
    'deps': [dep_mathlibrary],
  },
  'cutseq_decoder': {'engine': ['game/cutseq/decoder.c']},
  'cutseq_pak': {
    'engine': ['game/cutseq/pak.c', 'core/memory.c'],
    'deps': [dep_zlib],
  },
  'hash': {'engine': ['core/hash.c']},
  'bson': {
    'engine': [
      'core/bson/parse.c',
      'core/bson/write.c',
      'core/json/base.c',
      'core/memory.c',
    ],
  },
  'handle': {'engine': ['core/handle.c']},
  'item_actions': {'engine': ['game/items/actions/common.c']},
  'trigger': {
    'engine': ['game/items/trigger.c', 'version.c'],
    'deps': [dep_mathlibrary],
  },
  'console_complete': {
    'engine': [
      'core/completion.c',
      'game/console/completion.c',
      'game/console/registry.c',
      'core/memory.c',
      'core/vector.c',
    ],
  },
  'completion': {
    'engine': ['core/completion.c', 'core/memory.c', 'core/vector.c'],
  },
  'field': {'bundles': [b_value], 'engine': ['game/lua/field.c']},
  # The runtime override stack. The test answers the config layer for it, so no
  # option map, no settings file, no game.
  'config_override': {
    'bundles': [b_value],
    'engine': ['config/override.c', 'config/value.c'],
  },
  # The name<->value registry the Lua enum bridge reflects, and the dynamic
  # registry keyed on an opaque token. Both are narrower than b_value, so they
  # stand up only the half they are named for.
  'enum_map': {
    'engine': ['core/enum_map.c', 'core/memory.c', 'core/vector.c'],
    'src': ['stubs_enum_map.c'],
  },
  'dynamic_enum': {
    'engine': ['core/dynamic_enum.c', 'core/memory.c', 'core/vector.c'],
    'src': ['stubs_enum_map.c'],
  },
  'value': {'bundles': [b_value]},
  'lua_enum': {
    'bundles': [b_value],
    'engine': [
      'game/lua/capi/enum.c',
      'game/lua/registry.c',
      'game/lua/utils.c',
    ],
    'deps': [dep_lua],
  },
  # The sandbox depends on nothing but Lua, so the wall the engine stands up at
  # boot stands up here against a bare state - no modules, no engine.
  'lua_sandbox': {'engine': ['game/lua/sandbox.c'], 'deps': [dep_lua]},
  # The guard reads nothing but Lua and the clock, and the test supplies the
  # clock itself, stepped by hand, so the budget runs out deterministically.
  'lua_guard': {'engine': ['game/lua/guard.c'], 'deps': [dep_lua]},
  # field.c and lua/struct.c are leaf modules, so the reflection layer and the
  # Lua bridge can be exercised against a synthetic struct type.
  'lua_struct': {
    'bundles': [b_value],
    'engine': [
      'core/handle.c',
      'game/lua/capi/struct.c',
      'game/lua/field.c',
      'game/lua/registry.c',
      'game/lua/utils.c',
    ],
    'deps': [dep_lua],
  },
  'lua_rooms': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_rooms.c', 'fake_engine_items.c'],
    'engine': [
      'game/lua/capi/events.c',
      'game/lua/capi/items.c',
      'game/lua/capi/rooms.c',
    ],
  },
  'lua_items': {
    'bundles': [b_lua_surface, b_lua_strings],
    'src': ['fake_engine_items.c', 'fake_engine_catalog.c'],
    'engine': [
      'game/lua/capi/events.c',
      'game/lua/capi/items.c',
      'game/lua/capi/objects.c',
      'game/lua/capi/catalog.c',
    ],
  },
  # The same surface, taken through the two boot steps the other suites stop
  # short of: the seal, and hardening the globals.
  'lua_boot': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_items.c'],
    'engine': ['game/lua/capi/items.c'],
  },
  # No fake engine: the real trig tables are the thing under test.
  'lua_math': {
    'bundles': [b_lua_surface],
    'engine': ['game/lua/capi/math.c', 'core/math/trig.c'],
    'deps': [dep_mathlibrary],
  },
  'lua_assault': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_assault.c'],
    'engine': ['game/lua/capi/assault.c'],
  },
  # rooms rides along: trx.camera.room hands back a trx.rooms.Room.
  'lua_camera': {
    'bundles': [b_lua_surface],
    'src': [
      'fake_engine_camera.c',
      'fake_engine_rooms.c',
      # for Room_GetIndexFromPos, which rooms.c reaches for
      'fake_engine_items.c',
    ],
    'engine': ['game/lua/capi/rooms.c', 'game/lua/capi/camera.c'],
  },
  'lua_music': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_music.c'],
    'engine': ['game/lua/capi/music.c'],
  },
  'lua_strings': {'bundles': [b_lua_surface, b_lua_strings]},
  # The parser is pure Lua, but it leans on the real fuzzy matcher out of
  # trx.strings, so the two are stood up together.
  'lua_argparse': {
    'bundles': [b_lua_surface, b_lua_strings],
    'src': ['fake_engine_locale.c'],
    'engine': ['game/lua/capi/locale.c'],
  },
  'lua_sound': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_sound.c'],
    'engine': ['game/lua/capi/sound.c'],
  },
  'lua_locale': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_locale.c'],
    'engine': ['game/lua/capi/locale.c'],
  },
  'lua_lua': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_lua.c'],
    'engine': ['game/lua/capi/lua.c'],
  },
  'lua_weather': {
    'bundles': [b_lua_surface],
    'engine': ['game/lua/capi/weather.c'],
  },
  'lua_mod': {'bundles': [b_lua_surface], 'engine': ['game/lua/capi/mod.c']},
  'lua_savegame': {
    'bundles': [b_lua_surface],
    'engine': ['game/lua/capi/savegame.c'],
  },
  # No stubs: fake_engine_log.c provides Log_Message itself, because which line
  # the log blames is the thing under test.
  'lua_log': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_log.c'],
    'engine': ['game/lua/capi/log.c'],
    'stubs': false,
  },
  'lua_game': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_game.c'],
    'engine': ['game/lua/capi/game.c'],
  },
  'lua_objects': {
    'bundles': [b_lua_surface, b_lua_strings],
    'src': ['fake_engine_items.c', 'fake_engine_catalog.c'],
    'engine': ['game/lua/capi/catalog.c', 'game/lua/capi/objects.c'],
  },
  'lua_lara': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_lara.c', 'fake_engine_items.c'],
    'engine': ['game/lua/capi/items.c', 'game/lua/capi/lara.c'],
  },
  # The catalogs are real ENUM_MAP enums out of the real trx/game/enum.c. Only
  # the slot mapping, which a game loads from a CSV, is faked.
  'lua_catalog': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_catalog.c'],
    'engine': ['game/lua/capi/catalog.c'],
  },
  'lua_config': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_config.c', 'fake_engine_locale.c'],
    'engine': [
      'config/override.c',
      'config/value.c',
      'game/lua/capi/config.c',
      'game/lua/capi/locale.c',
    ],
  },
  'lua_console': {'bundles': [b_lua_surface, b_console, b_lua_strings]},
  # The help command, which the console module registers and composes from the
  # same parser a command answers --help from. It needs nothing the console
  # itself does not.
  'lua_cmd_help': {'bundles': [b_lua_surface, b_console, b_lua_strings]},
  # A console command written in Lua, exercised through the console that
  # dispatches it. Everything the command touches is a real bridge, so the test
  # says whether the command works, not whether it was written the way we
  # expected.
  'lua_cmd_heal': {
    'bundles': [b_lua_surface, b_console],
    'src': ['fake_engine_game.c', 'fake_engine_items.c', 'fake_engine_lara.c'],
    'engine': [
      'game/lua/capi/game.c',
      'game/lua/capi/items.c',
      'game/lua/capi/lara.c',
    ],
  },
  # The commands with non-trivial arguments, exercised through the console. Name
  # matching goes through fake_engine_strings, a pcre2-free stand-in, so these
  # stay light: the real matcher is covered by lua_strings and lua_argparse.
  'lua_cmd_play': {
    'bundles': [b_lua_surface, b_console],
    'src': ['fake_engine_game.c', 'fake_engine_strings.c'],
    'engine': ['game/lua/capi/game.c'],
  },
  'lua_cmd_music': {
    'bundles': [b_lua_surface, b_console],
    'src': ['fake_engine_music.c', 'fake_engine_strings.c'],
    'engine': ['game/lua/capi/music.c'],
  },
  'lua_cmd_load': {
    'bundles': [b_lua_surface, b_console],
    'src': ['fake_engine_savegame.c'],
  },
  'lua_cmd_rule': {
    'bundles': [b_lua_surface, b_console, b_lua_strings],
    'engine': ['game/lua/capi/rules.c', 'game/rules.c'],
  },
  'lua_cmd_save': {
    'bundles': [b_lua_surface, b_console],
    'src': ['fake_engine_game.c', 'fake_engine_savegame.c'],
    'engine': ['game/lua/capi/game.c'],
  },
  'lua_cmd_set': {
    'bundles': [b_lua_surface, b_console, b_lua_strings],
    'src': ['fake_engine_config.c'],
    'engine': [
      'config/override.c',
      'config/value.c',
      'game/lua/capi/config.c',
    ],
  },
  'lua_creatures': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_creatures.c'],
    'engine': ['game/lua/capi/creatures.c'],
  },
  'lua_events': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_rooms.c'],
    'engine': ['game/lua/capi/events.c'],
  },
  'lua_api_seal': {
    'bundles': [b_lua_surface],
    'src': ['fake_engine_items.c'],
    'engine': ['game/lua/capi/items.c'],
  },
  # The UI layout test is the widest of the unit tests: it stands up the settings
  # dialogs and runs the real measure and layout passes over them. It reads the
  # shipped font.bin for glyph metrics and the shipped config for the strings -
  # both versioned in this repository - so it still needs no window and no build
  # of the game.
  'ui_layout': {
    'src': [
      'fake_engine_settings.c',
      'fake_engine_ui.c',
      'fake_engine_ui_draw.c',
      'font_bin.c',
    ],
    'engine': [
      'config/common.c',
      'config/enum.c',
      'config/map.c',
      'config/override.c',
      'config/presets.c',
      'config/value.c',
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
      'core/strings/case_funcs.c',
      'core/strings/common.c',
      'core/value.c',
      'core/vector.c',
      'game/enum.c',
      'game/game_strings/entries.c',
      'game/gun/vars.c',
      'game/objects/vars.c',
      'game/rules.c',
      'game/ui/common.c',
      'game/ui/dialogs/color_editor.c',
      'game/ui/dialogs/config_presets.c',
      'game/ui/dialogs/gameplay_settings.c',
      'game/ui/dialogs/graphic_settings.c',
      'game/ui/dialogs/setting_helpers/enums.c',
      'game/ui/dialogs/setting_helpers/handlers.c',
      'game/ui/dialogs/setting_helpers/handlers_language.c',
      'game/ui/dialogs/settings.c',
      'game/ui/dialogs/settings_editor.c',
      'game/ui/dialogs/settings_tabs.c',
      'game/ui/dialogs/sound_settings.c',
      'game/ui/dialogs/text.c',
      'game/ui/events.c',
      'game/ui/helpers.c',
      'game/ui/scaler.c',
      'game/ui/scrollable.c',
      'game/ui/settings.c',
      'game/ui/text.c',
      'version.c',
      'gl/enum.c',
      'game/ui/elements/ammo_label.c',
      'game/ui/elements/anchor.c',
      'game/ui/elements/bar.c',
      'game/ui/elements/bar_enemy_hp.c',
      'game/ui/elements/bar_lara_air.c',
      'game/ui/elements/bar_lara_exposure.c',
      'game/ui/elements/bar_lara_hp.c',
      'game/ui/elements/bar_lara_sprint.c',
      'game/ui/elements/color_swatch.c',
      'game/ui/elements/flash.c',
      'game/ui/elements/fps_counter.c',
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
      'game/ui/elements/tab_switch.c',
      'game/ui/elements/window.c',
    ],
    'deps': [dep_mathlibrary, dep_pcre2, dep_zlib],
    'args': [
      '-DPCRE2_CODE_UNIT_WIDTH=8',
      '-DTEST_SHIP_CFG_DIR="' + repo_root / 'data/trx/ship/cfg' + '"',
      '-DTEST_SHIP_GAMES_DIR="' + repo_root / 'data/trx/ship/games' + '"',
    ],
  },
}

foreach name, spec : unit_tests
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

  sources = files('unit/test_' + name + '.c')
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
      sources += files('unit' / source)
    endif
  endforeach
  seen = []
  foreach source : test_engine
    if source not in seen
      seen += source
      sources += files('..' / 'trx' / source)
    endif
  endforeach

  exe = executable(
    'test_' + name,
    sources,
    include_directories: [src_inc, test_inc],
    dependencies: test_deps,
    c_args: test_args,
    build_by_default: false,
  )
  test(name, exe, suite: 'unit')
endforeach

lua_exe = find_program('lua', 'lua5.4', 'lua5.5', required: false)
if lua_exe.found()
  test('api', lua_exe, args: [files('unit/test_api.lua'), repo_root], suite: 'unit')
endif

python_exe = find_program('python3', required: false)
if python_exe.found()
  test('lua_sources', python_exe, args: [files('unit/test_lua_sources.py')], suite: 'unit')
  test('update_lua_docs', python_exe, args: [files('unit/test_update_lua_docs.py')], suite: 'unit')
  test(
    'game_strings',
    python_exe,
    args: [files('unit/test_game_strings.py')],
    suite: 'unit',
  )
endif
