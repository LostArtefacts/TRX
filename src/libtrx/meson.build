project(
  'libtrx',
  'c',
  default_options: [
    'c_std=c11',
    'warning_level=2',
  ],
)

staticdeps = get_option('staticdeps')
tr_version = get_option('tr_version')
gfx_gl_default_backend = get_option('gfx_gl_default_backend')

fs = import('fs')
c_compiler = meson.get_compiler('c')
relative_dir = fs.relative_to(meson.current_source_dir(), meson.global_build_root())
build_opts = [
  '-fmacro-prefix-map=@0@/=libtrx/'.format(relative_dir),
  '-Wno-unused',
  '-Wno-address-of-packed-member',
  '-DMESON_BUILD',
  '-DDWST_STATIC',
  '-DPCRE2_STATIC',
  '-DPCRE2_CODE_UNIT_WIDTH=8',
  '-DTR_VERSION=' + tr_version.to_string(),
  '-DGFX_GL_DEFAULT_BACKEND=' + gfx_gl_default_backend,
]
set_variable('defines', ['-DTR_VERSION=' + tr_version.to_string()])

add_project_arguments(build_opts, language: 'c')

# Always dynamically link on macOS
if host_machine.system() == 'darwin'
  staticdeps = false
endif

uthash = subproject('uthash', default_options: ['warning_level=0'])

null_dep = dependency('', required: false)
dep_avcodec = dependency('libavcodec', static: staticdeps)
dep_avformat = dependency('libavformat', static: staticdeps)
dep_avutil = dependency('libavutil', static: staticdeps)
dep_sdl2 = dependency('SDL2', static: staticdeps)
dep_pcre2 = dependency('libpcre2-8', static: staticdeps)
dep_backtrace = c_compiler.find_library('backtrace', static: true, required: false)
dep_swscale = dependency('libswscale', static: staticdeps)
dep_swresample = dependency('libswresample', static: staticdeps)

dep_zlib = null_dep
if not staticdeps
  dep_zlib = dependency('zlib', static: staticdeps)
endif

if host_machine.system() == 'windows'
  dep_opengl = c_compiler.find_library('opengl32')
else
  dep_opengl = dependency('GL')
endif

sources = [
  'benchmark.c',
  'config/common.c',
  'config/file.c',
  'engine/audio.c',
  'engine/audio_sample.c',
  'engine/audio_stream.c',
  'engine/image.c',
  'enum_map.c',
  'event_manager.c',
  'filesystem.c',
  'game/backpack.c',
  'game/console/cmd/config.c',
  'game/console/cmd/die.c',
  'game/console/cmd/end_level.c',
  'game/console/cmd/exit_game.c',
  'game/console/cmd/exit_to_title.c',
  'game/console/cmd/flipmap.c',
  'game/console/cmd/fly.c',
  'game/console/cmd/give_item.c',
  'game/console/cmd/heal.c',
  'game/console/cmd/kill.c',
  'game/console/cmd/load_game.c',
  'game/console/cmd/play_demo.c',
  'game/console/cmd/play_level.c',
  'game/console/cmd/pos.c',
  'game/console/cmd/save_game.c',
  'game/console/cmd/set_health.c',
  'game/console/cmd/sfx.c',
  'game/console/cmd/teleport.c',
  'game/console/common.c',
  'game/game_string.c',
  'game/items.c',
  'game/objects/names.c',
  'game/ui/common.c',
  'game/ui/events.c',
  'game/ui/widgets/console.c',
  'game/ui/widgets/prompt.c',
  'game/ui/widgets/spacer.c',
  'game/ui/widgets/stack.c',
  'gfx/2d/2d_renderer.c',
  'gfx/2d/2d_surface.c',
  'gfx/3d/3d_renderer.c',
  'gfx/3d/vertex_stream.c',
  'gfx/context.c',
  'gfx/gl/buffer.c',
  'gfx/gl/gl_core_3_3.c',
  'gfx/gl/program.c',
  'gfx/gl/sampler.c',
  'gfx/gl/texture.c',
  'gfx/gl/utils.c',
  'gfx/gl/vertex_array.c',
  'gfx/renderers/fbo_renderer.c',
  'gfx/renderers/legacy_renderer.c',
  'gfx/screenshot.c',
  'json/bson_parse.c',
  'json/bson_write.c',
  'json/json_base.c',
  'json/json_parse.c',
  'json/json_write.c',
  'log.c',
  'memory.c',
  'strings/common.c',
  'strings/fuzzy_match.c',
  'vector.c',
  'virtual_file.c',
]

dependencies = [
  dep_avcodec,
  dep_avformat,
  dep_avutil,
  dep_sdl2,
  dep_pcre2,
  dep_backtrace,
  dep_swresample,
  dep_swscale,
  dep_zlib,
  dep_opengl,
  uthash.get_variable('uthash_dep'),
]

if dep_backtrace.found() and host_machine.system() == 'linux'
  sources += ['log_linux.c']
elif host_machine.system() == 'windows'
  sources += ['log_windows.c']
  dwarfstack = subproject('dwarfstack', default_options: ['warning_level=0'])
  dep_dwarfstack = dwarfstack.get_variable('dep_dwarfstack')
  dep_dbghelp = c_compiler.find_library('dbghelp')
  dependencies += [dep_dbghelp, dep_dwarfstack]
else
  sources += ['log_unknown.c']
endif

libtrx = static_library(
  'libtrx',
  sources,
  dependencies: dependencies,
  include_directories: ['.', 'include/libtrx/'],
)

dep_trx = declare_dependency(
  link_whole: libtrx,
  include_directories: [
    include_directories('include/', is_system: true)
  ]
)
