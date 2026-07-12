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

# The engine takes the dependency name from -Dlua_dep; standing alone, this
# project has to find Lua itself, and distributions disagree on the name.
dep_lua = dependency('lua5.4', 'lua-5.4', 'lua54', 'lua')

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
