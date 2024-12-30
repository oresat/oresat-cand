libcommon_files = [
  'str2buf.c',
]

libcommon_includes = include_directories('.')

libcommon = shared_library(
  'common',
  libcommon_files,
  include_directories: libcommon_includes,
  install : false,
)

libcommon_dep = declare_dependency(
  include_directories: libcommon_includes,
  link_with: libcommon
)
