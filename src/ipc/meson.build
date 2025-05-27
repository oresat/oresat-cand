libipc_files = [
  'ipc.c',
  'ipc_broadcaster.c',
  'ipc_consumer.c',
  'ipc_responder.c',
]

libipc_includes = include_directories([
  '.',
])

libipc = shared_library(
  'ipc',
  libipc_files,
  include_directories: libipc_includes,
  dependencies: [
    dependency('libzmq'),
    libcommon_dep,
    libcanopenlinux_dep,
    libodextensions_dep,
  ],
  install: false,
)

libipc_dep = declare_dependency(
  include_directories: libipc_includes,
  link_with: libipc
)
