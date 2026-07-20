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
add_project_arguments(['-Wno-unused', '-Wno-unused-parameter'], language: 'c')

# The engine takes the dependency name from -Dlua_dep; standing alone, this
# project has to find Lua itself, and distributions disagree on the name.
dep_lua = dependency('lua5.4', 'lua-5.4', 'lua54', 'lua')

# core/strings does its regex with PCRE2.
dep_pcre2 = dependency('libpcre2-8')

# enum_map.c hashes with uthash, which is headers only.
c_compiler.check_header('uthash.h', required: true)

# <trx/...> resolves against src/; the harness header against src/tests/unit/.
src_inc = include_directories('..')
test_inc = include_directories('unit')
test_stubs = files('unit/stubs.c')

test_vector_exe = executable(
  'test_vector',
  files('unit/test_vector.c') + test_stubs
    + files('../trx/core/vector.c', '../trx/core/memory.c'),
  include_directories: [src_inc, test_inc],
  build_by_default: false,
)
test('vector', test_vector_exe, suite: 'unit')

test_math_func_exe = executable(
  'test_math_func',
  files('unit/test_math_func.c') + test_stubs
    + files('../trx/core/math/func.c', '../trx/core/math/geom.c', '../trx/core/math/trig.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_mathlibrary],
  build_by_default: false,
)
test('math_func', test_math_func_exe, suite: 'unit')

test_hash_exe = executable(
  'test_hash',
  files('unit/test_hash.c') + test_stubs + files('../trx/core/hash.c'),
  include_directories: [src_inc, test_inc],
  build_by_default: false,
)
test('hash', test_hash_exe, suite: 'unit')

test_handle_exe = executable(
  'test_handle',
  files('unit/test_handle.c') + test_stubs + files('../trx/core/handle.c'),
  include_directories: [src_inc, test_inc],
  build_by_default: false,
)
test('handle', test_handle_exe, suite: 'unit')

test_field_exe = executable(
  'test_field',
  files('unit/test_field.c') + test_stubs + files('../trx/game/lua/field.c'),
  include_directories: [src_inc, test_inc],
  build_by_default: false,
)
test('field', test_field_exe, suite: 'unit')

# The runtime override stack. The test answers the config layer for it, so no
# option map, no settings file, no game.
test_config_override_exe = executable(
  'test_config_override',
  files('unit/test_config_override.c') + test_stubs
    + files(
      '../trx/config/override.c',
      '../trx/config/value.c',
      '../trx/core/memory.c',
      '../trx/core/vector.c',
    ),
  include_directories: [src_inc, test_inc],
  build_by_default: false,
)
test('config_override', test_config_override_exe, suite: 'unit')

# The name<->value registry the Lua enum bridge reflects. Its two dependencies
# outside core - the formatting helper and the localized game strings - are
# stubbed: the real ones live in translation units that want PCRE2 and the game
# string table, and neither is what the enum map is being tested for.
enum_map_src = files(
  '../trx/core/enum_map.c',
  '../trx/core/memory.c',
  '../trx/core/vector.c',
) + files('unit/stubs_enum_map.c')

test_enum_map_exe = executable(
  'test_enum_map',
  files('unit/test_enum_map.c') + test_stubs + enum_map_src,
  include_directories: [src_inc, test_inc],
  build_by_default: false,
)
test('enum_map', test_enum_map_exe, suite: 'unit')

test_lua_enum_exe = executable(
  'test_lua_enum',
  files('unit/test_lua_enum.c') + test_stubs + enum_map_src
    + files(
      '../trx/game/lua/capi/enum.c',
      '../trx/game/lua/registry.c',
      '../trx/game/lua/utils.c',
    ),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  build_by_default: false,
)
test('lua_enum', test_lua_enum_exe, suite: 'unit')

# The sandbox depends on nothing but Lua, so the wall the engine stands up at
# boot stands up here against a bare state - no modules, no engine.
test_lua_sandbox_exe = executable(
  'test_lua_sandbox',
  files('unit/test_lua_sandbox.c') + test_stubs + files('../trx/game/lua/sandbox.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  build_by_default: false,
)
test('lua_sandbox', test_lua_sandbox_exe, suite: 'unit')

# The guard reads nothing but Lua and the clock, and the test supplies the
# clock itself, stepped by hand, so the budget runs out deterministically.
test_lua_guard_exe = executable(
  'test_lua_guard',
  files('unit/test_lua_guard.c') + test_stubs + files('../trx/game/lua/guard.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  build_by_default: false,
)
test('lua_guard', test_lua_guard_exe, suite: 'unit')

# The public Lua surface, driven the way a level script drives it. Everything
# below the assertions is real - the FIELD_DESC tables, the struct and enum
# bridges, the trxc modules, and src/lua/api/*.lua read from the tree - and
# only the engine underneath is faked. The modules are read from
# disk rather than from the embedded copy, so the tests need to know where the
# tree is.
lua_surface_src = files(
  '../trx/core/enum_map.c',
  '../trx/core/handle.c',
  '../trx/core/memory.c',
  '../trx/core/vector.c',
  '../trx/game/enum.c',
  '../trx/game/lua/capi/api.c',
  '../trx/game/lua/capi/enum.c',
  '../trx/game/lua/capi/struct.c',
  '../trx/game/lua/field.c',
  '../trx/game/lua/registry.c',
  '../trx/game/lua/sandbox.c',
  '../trx/game/lua/utils.c',
) + files('unit/lua_surface.c', 'unit/stubs_enum_map.c')

lua_surface_args = ['-DREPO_ROOT="' + meson.current_source_dir() / '../..' + '"']

test_lua_rooms_exe = executable(
  'test_lua_rooms',
  files('unit/test_lua_rooms.c', 'unit/fake_engine_rooms.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/rooms.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_rooms', test_lua_rooms_exe, suite: 'unit')

test_lua_items_exe = executable(
  'test_lua_items',
  files('unit/test_lua_items.c', 'unit/fake_engine_items.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/items.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_items', test_lua_items_exe, suite: 'unit')

# The same surface, taken through the two boot steps the other suites stop short
# of: the seal, and hardening the globals.
test_lua_boot_exe = executable(
  'test_lua_boot',
  files('unit/test_lua_boot.c', 'unit/fake_engine_items.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/items.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_boot', test_lua_boot_exe, suite: 'unit')

# No fake engine: the real trig tables are the thing under test.
test_lua_math_exe = executable(
  'test_lua_math',
  files('unit/test_lua_math.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/math.c', '../trx/core/math/trig.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua, dep_mathlibrary],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_math', test_lua_math_exe, suite: 'unit')

test_lua_assault_exe = executable(
  'test_lua_assault',
  files('unit/test_lua_assault.c', 'unit/fake_engine_assault.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/assault.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_assault', test_lua_assault_exe, suite: 'unit')

# rooms rides along: trx.camera.room hands back a trx.rooms.Room.
test_lua_camera_exe = executable(
  'test_lua_camera',
  files('unit/test_lua_camera.c', 'unit/fake_engine_camera.c', 'unit/fake_engine_rooms.c')
    + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/rooms.c', '../trx/game/lua/capi/camera.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_camera', test_lua_camera_exe, suite: 'unit')

test_lua_music_exe = executable(
  'test_lua_music',
  files('unit/test_lua_music.c', 'unit/fake_engine_music.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/music.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_music', test_lua_music_exe, suite: 'unit')

# The fuzzy matcher and the regex are the real ones out of trx/core/strings.
test_lua_strings_exe = executable(
  'test_lua_strings',
  files('unit/test_lua_strings.c') + test_stubs
    + lua_surface_src
    + files(
      '../trx/core/strings/common.c',
      '../trx/core/strings/case_funcs.c',
      '../trx/core/strings/fuzzy_match.c',
      '../trx/game/lua/capi/strings.c',
    ),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua, dep_pcre2],
  c_args: lua_surface_args + ['-DPCRE2_CODE_UNIT_WIDTH=8'],
  build_by_default: false,
)
test('lua_strings', test_lua_strings_exe, suite: 'unit')

test_lua_sound_exe = executable(
  'test_lua_sound',
  files('unit/test_lua_sound.c', 'unit/fake_engine_sound.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/sound.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_sound', test_lua_sound_exe, suite: 'unit')

test_lua_locale_exe = executable(
  'test_lua_locale',
  files('unit/test_lua_locale.c', 'unit/fake_engine_locale.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/locale.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_locale', test_lua_locale_exe, suite: 'unit')

test_lua_weather_exe = executable(
  'test_lua_weather',
  files('unit/test_lua_weather.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/weather.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_weather', test_lua_weather_exe, suite: 'unit')

# No test_stubs: fake_engine_log.c provides Log_Message itself, because which
# line the log blames is the thing under test.
test_lua_log_exe = executable(
  'test_lua_log',
  files('unit/test_lua_log.c', 'unit/fake_engine_log.c')
    + lua_surface_src
    + files('../trx/game/lua/capi/log.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_log', test_lua_log_exe, suite: 'unit')

test_lua_game_exe = executable(
  'test_lua_game',
  files('unit/test_lua_game.c', 'unit/fake_engine_game.c') + test_stubs
    + lua_surface_src
    + files(
      '../trx/game/lua/capi/game.c',
    ),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_game', test_lua_game_exe, suite: 'unit')

test_lua_objects_exe = executable(
  'test_lua_objects',
  files('unit/test_lua_objects.c', 'unit/fake_engine_items.c', 'unit/fake_engine_catalog.c')
    + test_stubs
    + lua_surface_src
    + files(
      '../trx/core/strings/case_funcs.c',
      '../trx/core/strings/common.c',
      '../trx/core/strings/fuzzy_match.c',
      '../trx/game/lua/capi/catalog.c',
      '../trx/game/lua/capi/objects.c',
      '../trx/game/lua/capi/strings.c',
    ),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua, dep_pcre2],
  c_args: lua_surface_args + ['-DPCRE2_CODE_UNIT_WIDTH=8'],
  build_by_default: false,
)
test('lua_objects', test_lua_objects_exe, suite: 'unit')

test_lua_lara_exe = executable(
  'test_lua_lara',
  files('unit/test_lua_lara.c', 'unit/fake_engine_lara.c', 'unit/fake_engine_items.c')
    + test_stubs
    + lua_surface_src
    + files(
      '../trx/game/lua/capi/items.c',
      '../trx/game/lua/capi/lara.c',
    ),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_lara', test_lua_lara_exe, suite: 'unit')

# The catalogs are real ENUM_MAP enums out of the real trx/game/enum.c. Only the
# slot mapping, which a game loads from a CSV, is faked.
test_lua_catalog_exe = executable(
  'test_lua_catalog',
  files('unit/test_lua_catalog.c', 'unit/fake_engine_catalog.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/catalog.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_catalog', test_lua_catalog_exe, suite: 'unit')

test_lua_config_exe = executable(
  'test_lua_config',
  files('unit/test_lua_config.c', 'unit/fake_engine_config.c') + test_stubs
    + lua_surface_src
    + files(
      '../trx/config/override.c',
      '../trx/config/value.c',
      '../trx/game/lua/capi/config.c',
    ),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_config', test_lua_config_exe, suite: 'unit')

test_lua_console_exe = executable(
  'test_lua_console',
  files('unit/test_lua_console.c', 'unit/fake_engine_console.c') + test_stubs
    + lua_surface_src
    + files(
      '../trx/game/lua/capi/console.c',
      '../trx/game/lua/capi/log.c',
    ),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_console', test_lua_console_exe, suite: 'unit')

# A console command written in Lua, exercised through the console that dispatches
# it. Everything the command touches is a real bridge, so the test says whether
# the command works, not whether it was written the way we expected.
test_lua_cmd_heal_exe = executable(
  'test_lua_cmd_heal',
  files(
    'unit/test_lua_cmd_heal.c',
    'unit/fake_engine_console.c',
    'unit/fake_engine_game.c',
    'unit/fake_engine_items.c',
    'unit/fake_engine_lara.c',
    'unit/fake_engine_locale.c',
  ) + test_stubs
    + lua_surface_src
    + files(
      '../trx/game/lua/capi/console.c',
      '../trx/game/lua/capi/game.c',
      '../trx/game/lua/capi/items.c',
      '../trx/game/lua/capi/lara.c',
      '../trx/game/lua/capi/locale.c',
      '../trx/game/lua/capi/log.c',
    ),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_cmd_heal', test_lua_cmd_heal_exe, suite: 'unit')

test_lua_creatures_exe = executable(
  'test_lua_creatures',
  files('unit/test_lua_creatures.c', 'unit/fake_engine_creatures.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/creatures.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_creatures', test_lua_creatures_exe, suite: 'unit')

test_lua_events_exe = executable(
  'test_lua_events',
  files('unit/test_lua_events.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/events.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_events', test_lua_events_exe, suite: 'unit')

test_lua_api_seal_exe = executable(
  'test_lua_api_seal',
  files('unit/test_lua_api_seal.c', 'unit/fake_engine_items.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/capi/items.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_api_seal', test_lua_api_seal_exe, suite: 'unit')

# field.c and lua/struct.c are leaf modules, so the reflection layer and the Lua
# bridge can be exercised against a synthetic struct type.
test_lua_struct_exe = executable(
  'test_lua_struct',
  files('unit/test_lua_struct.c') + test_stubs
    + files(
      '../trx/core/handle.c',
      '../trx/core/memory.c',
      '../trx/game/lua/capi/struct.c',
      '../trx/game/lua/field.c',
      '../trx/game/lua/registry.c',
      '../trx/game/lua/utils.c',
    ),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  build_by_default: false,
)
test('lua_struct', test_lua_struct_exe, suite: 'unit')

# api.lua is read from the repository root, two levels up from this project.
repo_root = meson.current_source_dir() / '../..'

lua_exe = find_program('lua', 'lua5.4', 'lua5.5', required: false)
if lua_exe.found()
  test('api', lua_exe, args: [files('unit/test_api.lua'), repo_root], suite: 'unit')
endif

python_exe = find_program('python3', required: false)
if python_exe.found()
  test('lua_sources', python_exe, args: [files('unit/test_lua_sources.py')], suite: 'unit')
  test('update_lua_docs', python_exe, args: [files('unit/test_update_lua_docs.py')], suite: 'unit')
  test(
    'update_game_strings',
    python_exe,
    args: [files('unit/test_update_game_strings.py')],
    suite: 'unit',
  )
endif
