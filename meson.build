project('untitled-video-game', 'c', default_options : ['c_std=c17'])

cc = meson.get_compiler('c')
if cc.get_id() != 'clang'
  error('This project is configured to build with Clang. Reconfigure Meson with CC=clang.')
endif

glfw = dependency('glfw3')

src_inc = include_directories('src')
wgpu_inc = include_directories('external/wgpu-native/include')
wgpu_lib_dir = meson.project_source_root() / 'external' / 'wgpu-native' / 'lib'
wgpu_native = cc.find_library(
  'wgpu_native',
  dirs : [wgpu_lib_dir],
  required : true,
)

executable(
  'untitled-video-game',
  [
    'src/main.c',
    'src/app/app.c',
    'src/gfx/gfx.c',
    'src/utils/error.c',
    'src/utils/logger.c',
  ],
  dependencies : [glfw, wgpu_native],
  include_directories : [src_inc, wgpu_inc],
)
