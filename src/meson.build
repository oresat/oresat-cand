subdir('common')
subdir('canopenlinux')
subdir('od_extensions')

libcanopend_files = [
  'conf_load.c',
  'ipc.c',
  'main.c',
]

libcanopend_includes = include_directories([
  '.',
])

libcanopend = shared_library(
  'canopend',
  libcanopend_files,
  include_directories: libcanopend_includes,
  dependencies: [
    dependency('threads'),
    dependency('libzmq'),
    libcommon_dep,
    libcanopenlinux_dep,
    libodextensions_dep,
  ],
  install: false,
  c_args: [
    '-DCO_MULTIPLE_OD',
    '-DCO_DRIVER_CUSTOM',
    '-DPROJECT_NAME="' + meson.project_name() + '"',
    '-DPROJECT_VERSION="' + meson.project_version() + '"',
  ]
)

libcanopend_dep = declare_dependency(
  include_directories: libcanopend_includes,
  link_with: libcanopend
)
