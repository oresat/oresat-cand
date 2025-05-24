libcommon_files = [
  'ecss_time.c',
  'fcache.c',
  'logger.c',
  'str2buf.c',
  'system.c',
]

libcommon_includes = include_directories('.')

libcommon = shared_library(
  'common',
  libcommon_files,
  include_directories: libcommon_includes,
  dependencies: [
    dependency('zlib'),
  ],
  install : false,
)

libcommon_dep = declare_dependency(
  include_directories: libcommon_includes,
  link_with: libcommon
)
