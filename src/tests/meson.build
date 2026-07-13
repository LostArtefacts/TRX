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

test_field_exe = executable(
  'test_field',
  files('unit/test_field.c') + test_stubs + files('../trx/core/field.c'),
  include_directories: [src_inc, test_inc],
  build_by_default: false,
)
test('field', test_field_exe, suite: 'unit')

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
    + files('../trx/game/lua/enum.c'),
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

# The public Lua surface, driven the way a level script drives it. Everything
# below the assertions is real - the FIELD_DESC tables, the struct and enum
# bridges, the trxc modules, and data/scripting/*.lua read from the tree - and
# only the engine underneath is faked. The modules are read from disk rather than
# from the embedded copy, so the tests need to know where the tree is.
lua_surface_src = files(
  '../trx/core/enum_map.c',
  '../trx/core/field.c',
  '../trx/core/memory.c',
  '../trx/core/vector.c',
  '../trx/game/enum.c',
  '../trx/game/lua/enum.c',
  '../trx/game/lua/struct.c',
) + files('unit/lua_surface.c', 'unit/stubs_enum_map.c')

lua_surface_args = ['-DREPO_ROOT="' + meson.current_source_dir() / '../..' + '"']

test_lua_rooms_exe = executable(
  'test_lua_rooms',
  files('unit/test_lua_rooms.c', 'unit/fake_engine_rooms.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/rooms/fields.c', '../trx/game/lua/rooms.c'),
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
    + files(
      '../trx/game/items/fields.c',
      '../trx/game/lua/items.c',
      '../trx/game/lua/utils.c',
    ),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_items', test_lua_items_exe, suite: 'unit')

test_lua_assault_exe = executable(
  'test_lua_assault',
  files('unit/test_lua_assault.c', 'unit/fake_engine_assault.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/assault.c'),
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
    + files('../trx/game/rooms/fields.c', '../trx/game/lua/rooms.c', '../trx/game/lua/camera.c'),
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
    + files('../trx/game/lua/music.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_music', test_lua_music_exe, suite: 'unit')

test_lua_sound_exe = executable(
  'test_lua_sound',
  files('unit/test_lua_sound.c', 'unit/fake_engine_sound.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/sound.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_sound', test_lua_sound_exe, suite: 'unit')

# No test_stubs: fake_engine_log.c provides Log_Message itself, because which
# line the log blames is the thing under test.
test_lua_log_exe = executable(
  'test_lua_log',
  files('unit/test_lua_log.c', 'unit/fake_engine_log.c')
    + lua_surface_src
    + files('../trx/game/lua/log.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_log', test_lua_log_exe, suite: 'unit')

# The log bridge rides along: console.lua takes its levels from trxc.log.
test_lua_console_exe = executable(
  'test_lua_console',
  files('unit/test_lua_console.c', 'unit/fake_engine_console.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/console.c', '../trx/game/lua/log.c'),
  include_directories: [src_inc, test_inc],
  dependencies: [dep_lua],
  c_args: lua_surface_args,
  build_by_default: false,
)
test('lua_console', test_lua_console_exe, suite: 'unit')

test_lua_creatures_exe = executable(
  'test_lua_creatures',
  files('unit/test_lua_creatures.c', 'unit/fake_engine_creatures.c') + test_stubs
    + lua_surface_src
    + files('../trx/game/lua/creatures.c'),
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
    + files('../trx/game/lua/events.c'),
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
    + files(
      '../trx/game/items/fields.c',
      '../trx/game/lua/items.c',
      '../trx/game/lua/utils.c',
    ),
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
    + files('../trx/core/field.c', '../trx/game/lua/struct.c'),
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
  test('update_lua_docs', python_exe, args: [files('unit/test_update_lua_docs.py')], suite: 'unit')
endif
