libcanopenlinux_files = [
  'CANopenLinux/CANopenNode/CANopen.c',
  'CANopenLinux/CANopenNode/301/CO_Emergency.c',
  'CANopenLinux/CANopenNode/301/CO_fifo.c',
  'CANopenLinux/CANopenNode/301/CO_HBconsumer.c',
  'CANopenLinux/CANopenNode/301/CO_NMT_Heartbeat.c',
  'CANopenLinux/CANopenNode/301/CO_Node_Guarding.c',
  'CANopenLinux/CANopenNode/301/CO_ODinterface.c',
  'CANopenLinux/CANopenNode/301/CO_PDO.c',
  'CANopenLinux/CANopenNode/301/CO_SDOclient.c',
  'CANopenLinux/CANopenNode/301/CO_SDOserver.c',
  'CANopenLinux/CANopenNode/301/CO_SYNC.c',
  'CANopenLinux/CANopenNode/301/CO_TIME.c',
  'CANopenLinux/CANopenNode/301/crc16-ccitt.c',
  'CANopenLinux/CO_driver.c',
  'CANopenLinux/CO_epoll_interface.c',
  'CANopenLinux/CO_error.c',
  'OD.c',
  'log_prinf.c',
]

libcanopenlinux_includes = include_directories([
  '.',
  'CANopenLinux',
  'CANopenLinux/CANopenNode',
  'CANopenLinux/CANopenNode/301',
])

libcanopenlinux = shared_library(
  'CANopenLinux',
  libcanopenlinux_files,
  include_directories: libcanopenlinux_includes,
  dependencies: dependency('threads'),
  install: false,
  c_args: '-DCO_DRIVER_CUSTOM',
)

libcanopenlinux_dep = declare_dependency(
  include_directories: libcanopenlinux_includes,
  link_with: libcanopenlinux
)
