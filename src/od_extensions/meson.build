libodextensions_files = [
  'od_ext.c',
  'os_command_ext.c',
]

libodextensions_includes = include_directories('.')

libodextensions = shared_library(
  'od-extensions',
  libodextensions_files,
  include_directories: libodextensions_includes,
  dependencies: [
    libcommon_dep,
    libcanopenlinux_dep,
  ],
  install : false,
)

libodextensions_dep = declare_dependency(
  include_directories: libodextensions_includes,
  link_with: libodextensions
)
