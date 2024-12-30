project(
  'oresat-canopend',
  'c',
  version : '0', # run_command('git', 'describe').stdout.strip(),
  license: 'GPL-3.0',
  default_options : [
    'warning_level=3',
    'strip=true',
  ]
)

build_args = [
  '-DCO_DRIVER_CUSTOM',
  '-DPROJECT_NAME=' + meson.project_name(),
  '-DPROJECT_VERSION=' + meson.project_version(),
]

subdir('src')

project_target = executable(
  meson.project_name(),
  'src/main.c',
  link_with: libcanopenlinux,
  dependencies: [
    dependency('threads'),
    libcanopenlinux_dep,
  ],
  install: true,
  c_args: build_args,
)

subdir('scripts')

project_target = executable(
  'oresat-sdo-read',
  'scripts/sdo_read_main.c',
  link_with: libsdoclient,
  dependencies: [
    libcanopenlinux_dep,
    libcommon_dep,
    libsdoclient_dep
  ],
  install: false,
  c_args: build_args,
)

project_target = executable(
  'oresat-sdo-write',
  'scripts/sdo_write_main.c',
  link_with: libsdoclient,
  dependencies: [
    libcanopenlinux_dep,
    libcommon_dep,
    libsdoclient_dep
  ],
  install: false,
  c_args: build_args,
)
