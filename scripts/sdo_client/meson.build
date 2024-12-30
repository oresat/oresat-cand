libsdoclient_files = [
  'basic_node.c',
  'sdo_client.c',
]

libsdoclient_includes = include_directories('.')

libsdoclient = shared_library(
  'sdoclient',
  libsdoclient_files,
  include_directories: libsdoclient_includes,
  dependencies: [
    libcanopenlinux_dep,
    libcommon_dep,
  ],
  install : false,
  c_args: [
  '-DCO_DRIVER_CUSTOM',
  '-DCO_SINGLE_THREAD',
  ],
)

libsdoclient_dep = declare_dependency(
  include_directories: libsdoclient_includes,
  link_with: libsdoclient
)
