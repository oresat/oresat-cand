libsdoclientnode_files = [
  'sdo_client_node.c',
]

libsdoclientnode_includes = include_directories('.')

libsdoclientnode = shared_library(
  'sdoclientnode',
  libsdoclientnode_files,
  include_directories: libsdoclientnode_includes,
  dependencies: [
    libcanopenlinux_dep,
    libcommon_dep,
  ],
  install : false,
  c_args: [
  '-DCO_DRIVER_CUSTOM',
  '-DCO_SINGLE_THREAD',
  '-DCO_MULTIPLE_OD',
  ],
)

libsdoclientnode_dep = declare_dependency(
  include_directories: libsdoclientnode_includes,
  link_with: libsdoclientnode
)
