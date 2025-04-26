project(
  'libtrx',
  'c',
  default_options: [
    'c_std=c2x',
    'warning_level=3',
  ],
  meson_version: '>=1.3.0',
)

staticdeps = get_option('staticdeps')
tr_version = get_option('tr_version')

fs = import('fs')
c_compiler = meson.get_compiler('c')
relative_dir = fs.relative_to(meson.current_source_dir(), meson.global_build_root())
build_opts = [
  # common options
  '-Wno-unused',
  '-Wno-unused-parameter',
  '-Wno-microsoft-anon-tag',
  '-Wno-gnu-binary-literal',
  '-Wno-gnu-empty-initializer',
  '-Wno-gnu-zero-variadic-macro-arguments',
  '-DMESON_BUILD',
  '-DGLEW_NO_GLU',
  '-DSDL_MAIN_HANDLED',
  '-fms-extensions',
  '-fno-omit-frame-pointer',
  # end of common options
  '-fmacro-prefix-map=@0@/=libtrx/'.format(relative_dir),
  '-DDWST_STATIC',
  '-DPCRE2_STATIC',
  '-DPCRE2_CODE_UNIT_WIDTH=8',
  '-DTR_VERSION=' + tr_version.to_string(),
  '-DDEBUG=' + (get_option('buildtype') == 'debug' ? '1' : '0'),
]
set_variable('defines', ['-DTR_VERSION=' + tr_version.to_string()])

add_project_arguments(build_opts, language: 'c')

# Always dynamically link on macOS
if host_machine.system() == 'darwin'
  staticdeps = false
endif

null_dep = dependency('', required: false)
dep_avcodec = dependency('libavcodec', static: staticdeps)
dep_avformat = dependency('libavformat', static: staticdeps)
dep_avutil = dependency('libavutil', static: staticdeps)
dep_sdl2 = dependency('SDL2', static: staticdeps)
dep_glew = dependency('glew', static: staticdeps)
dep_pcre2 = dependency('libpcre2-8', static: staticdeps)
dep_backtrace = c_compiler.find_library('backtrace', static: true, required: false)
dep_swscale = dependency('libswscale', static: staticdeps)
dep_swresample = dependency('libswresample', static: staticdeps)
c_compiler.check_header('uthash.h', required: true)

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
  'config/map.c',
  'config/priv.c',
  'config/vars.c',
  'engine/audio.c',
  'engine/audio_sample.c',
  'engine/audio_stream.c',
  'engine/image.c',
  'engine/video.c',
  'enum_map.c',
  'event_manager.c',
  'filesystem.c',
  'game/anims/commands.c',
  'game/anims/common.c',
  'game/anims/frames.c',
  'game/camera/cinematic.c',
  'game/camera/common.c',
  'game/camera/fixed.c',
  'game/camera/photo_mode.c',
  'game/camera/vars.c',
  'game/clock/common.c',
  'game/clock/timer.c',
  'game/clock/turbo.c',
  'game/collision.c',
  'game/console/cmd/config.c',
  'game/console/cmd/die.c',
  'game/console/cmd/end_level.c',
  'game/console/cmd/exit_game.c',
  'game/console/cmd/exit_to_title.c',
  'game/console/cmd/flipmap.c',
  'game/console/cmd/flood.c',
  'game/console/cmd/fly.c',
  'game/console/cmd/give_item.c',
  'game/console/cmd/heal.c',
  'game/console/cmd/kill.c',
  'game/console/cmd/load_game.c',
  'game/console/cmd/music.c',
  'game/console/cmd/play_cutscene.c',
  'game/console/cmd/play_demo.c',
  'game/console/cmd/play_gym.c',
  'game/console/cmd/play_level.c',
  'game/console/cmd/pos.c',
  'game/console/cmd/save_game.c',
  'game/console/cmd/set_health.c',
  'game/console/cmd/sfx.c',
  'game/console/cmd/speed.c',
  'game/console/cmd/teleport.c',
  'game/console/common.c',
  'game/console/history.c',
  'game/console/registry.c',
  'game/creature/common.c',
  'game/demo/common.c',
  'game/fader.c',
  'game/game.c',
  'game/game_buf.c',
  'game/game_flow/common.c',
  'game/game_flow/reader.c',
  'game/game_flow/sequencer.c',
  'game/game_flow/sequencer_events.c',
  'game/game_flow/sequencer_misc.c',
  'game/game_flow/vars.c',
  'game/game_string.c',
  'game/game_string_table/common.c',
  'game/game_string_table/priv.c',
  'game/game_string_table/reader.c',
  'game/gun/common.c',
  'game/gym.c',
  'game/inject/common.c',
  'game/inject/data/anims.c',
  'game/inject/data/meshes.c',
  'game/inject/data/objects.c',
  'game/inject/data/sound.c',
  'game/inject/data/textures.c',
  'game/inject/editor.c',
  'game/inject/editors/anims.c',
  'game/inject/editors/floor_data.c',
  'game/inject/editors/meshes.c',
  'game/inject/editors/positions.c',
  'game/inject/editors/rooms.c',
  'game/inject/editors/textures.c',
  'game/inject/testers/items.c',
  'game/inject/testers/rooms.c',
  'game/input/backends/controller.c',
  'game/input/backends/internal.c',
  'game/input/backends/keyboard.c',
  'game/input/common.c',
  'game/interpolation.c',
  'game/inventory.c',
  'game/inventory_ring/priv.c',
  'game/items.c',
  'game/lara/common.c',
  'game/lara/misc.c',
  'game/level/common.c',
  'game/level/faces.c',
  'game/level/settings.c',
  'game/math/trig.c',
  'game/math/util.c',
  'game/matrix.c',
  'game/music.c',
  'game/objects/common.c',
  'game/objects/creatures/bear.c',
  'game/objects/creatures/wolf.c',
  'game/objects/general/bridge_common.c',
  'game/objects/general/bridge_flat.c',
  'game/objects/general/bridge_tilt1.c',
  'game/objects/general/bridge_tilt2.c',
  'game/objects/general/door.c',
  'game/objects/general/drawbridge.c',
  'game/objects/general/trapdoor.c',
  'game/objects/names.c',
  'game/objects/traps/movable_block.c',
  'game/objects/vars.c',
  'game/output/background.c',
  'game/output/common.c',
  'game/output/textures.c',
  'game/packer.c',
  'game/pathing/box.c',
  'game/phase/executor.c',
  'game/phase/phase_cutscene.c',
  'game/phase/phase_demo.c',
  'game/phase/phase_game.c',
  'game/phase/phase_inventory.c',
  'game/phase/phase_pause.c',
  'game/phase/phase_photo_mode.c',
  'game/phase/phase_picture.c',
  'game/phase/phase_stats.c',
  'game/random.c',
  'game/rooms/common.c',
  'game/rooms/draw.c',
  'game/savegame/common.c',
  'game/scaler.c',
  'game/shell/common.c',
  'game/shell/main.c',
  'game/sound.c',
  'game/text.c',
  'game/ui/common.c',
  'game/ui/dialogs/base_passport.c',
  'game/ui/dialogs/controls.c',
  'game/ui/dialogs/controls_backend.c',
  'game/ui/dialogs/controls_editor.c',
  'game/ui/dialogs/examine_item.c',
  'game/ui/dialogs/new_game.c',
  'game/ui/dialogs/pause.c',
  'game/ui/dialogs/photo_mode.c',
  'game/ui/dialogs/play_any_level.c',
  'game/ui/dialogs/save_slot.c',
  'game/ui/dialogs/select_level.c',
  'game/ui/dialogs/stats.c',
  'game/ui/elements/anchor.c',
  'game/ui/elements/fade.c',
  'game/ui/elements/fixed.c',
  'game/ui/elements/flash.c',
  'game/ui/elements/frame.c',
  'game/ui/elements/hide.c',
  'game/ui/elements/label.c',
  'game/ui/elements/modal.c',
  'game/ui/elements/offset.c',
  'game/ui/elements/pad.c',
  'game/ui/elements/prompt.c',
  'game/ui/elements/requester.c',
  'game/ui/elements/resize.c',
  'game/ui/elements/spacer.c',
  'game/ui/elements/stack.c',
  'game/ui/elements/window.c',
  'game/ui/events.c',
  'game/ui/helpers.c',
  'game/ui/hud/console.c',
  'game/ui/hud/console_logs.c',
  'gfx/2d/2d_renderer.c',
  'gfx/2d/2d_surface.c',
  'gfx/3d/3d_renderer.c',
  'gfx/3d/vertex_stream.c',
  'gfx/context.c',
  'gfx/fade/fade_renderer.c',
  'gfx/gl/buffer.c',
  'gfx/gl/program.c',
  'gfx/gl/sampler.c',
  'gfx/gl/texture.c',
  'gfx/gl/track.c',
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
  'screenshot.c',
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
  dep_glew,
  dep_pcre2,
  dep_backtrace,
  dep_swresample,
  dep_swscale,
  dep_zlib,
  dep_opengl,
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
