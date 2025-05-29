project(
  'oresat-cand',
  'c',
  version: run_command('git', 'describe', '--tags', '--abbrev=0', capture: true, check: true).stdout().strip(),
  license: 'GPL-3.0',
  default_options: [
    'warning_level=3',
    'strip=true',
  ]
)

build_args = [
  '-DCO_MULTIPLE_OD',
  '-DCO_DRIVER_CUSTOM',
  '-DPROJECT_NAME="' + meson.project_name() + '"',
  '-DPROJECT_VERSION="' + meson.project_version() + '"',
]

run_command('oresat-configs', 'canopennode', 'od.yaml', '-d', 'src/canopenlinux', check: true)

subdir('src')

project_target = executable(
  meson.project_name(),
  'src/main.c',
  link_with: [
    libcanopenlinux,
    libcommon,
    libodextensions,
    libipc,
    libcand,
  ],
  dependencies: [
    dependency('threads'),
    libcommon_dep,
    libcanopenlinux_dep,
    libodextensions_dep,
    libipc_dep,
    libcand_dep,
  ],
  install: true,
  c_args: build_args,
)

subdir('scripts')

project_target = executable(
  'oresat-sdo-read',
  'scripts/sdo_read_main.c',
  link_with: libsdoclientnode,
  dependencies: [
    libcanopenlinux_dep,
    libcommon_dep,
    libodextensions_dep,
    libsdoclientnode_dep
  ],
  install: false,
  c_args: build_args,
)

project_target = executable(
  'oresat-sdo-write',
  'scripts/sdo_write_main.c',
  link_with: libsdoclientnode,
  dependencies: [
    libcanopenlinux_dep,
    libcommon_dep,
    libodextensions_dep,
    libsdoclientnode_dep
  ],
  install: false,
  c_args: build_args,
)

project_target = executable(
  'oresat-os-command',
  'scripts/os_command_main.c',
  link_with: libsdoclientnode,
  dependencies: [
    libcanopenlinux_dep,
    libcommon_dep,
    libodextensions_dep,
    libsdoclientnode_dep
  ],
  install: false,
  c_args: build_args,
)

project_target = executable(
  'oresat-fwrite',
  'scripts/fwrite_main.c',
  link_with: libsdoclientnode,
  dependencies: [
    libcanopenlinux_dep,
    libcommon_dep,
    libodextensions_dep,
    libsdoclientnode_dep
  ],
  install: false,
  c_args: build_args,
)

project_target = executable(
  'oresat-fread',
  'scripts/fread_main.c',
  link_with: libsdoclientnode,
  dependencies: [
    libcanopenlinux_dep,
    libcommon_dep,
    libodextensions_dep,
    libsdoclientnode_dep
  ],
  install: false,
  c_args: build_args,
)

project_target = executable(
  'oresat-fcache-list',
  'scripts/fcache_list_main.c',
  link_with: libsdoclientnode,
  dependencies: [
    libcanopenlinux_dep,
    libcommon_dep,
    libodextensions_dep,
    libsdoclientnode_dep
  ],
  install: false,
  c_args: build_args,
)

subdir('tests')
