project(
  'libanemo',
  'cpp',
  version : '0.1.0',
  default_options : ['cpp_std=c++20'],
  license : 'MulanPSL-2.0',
)

inc = include_directories('include')

libvio_src = files(
  'src/libvio/bus.cc',
  'src/libvio/frontend.cc',
  'src/libvio/console/backend_iostream.cc',
  'src/libvio/console/frontend.cc',
  'src/libvio/mtime/backend_chrono.cc',
  'src/libvio/mtime/frontend.cc',
)

libcpu_src = files(
  'src/libcpu/memory.cc',
)

libsdb_src = files(
  'src/libsdb/commandline.cc',
  'src/libsdb/expression.cc',
)

libanemo = static_library(
  'anemo',
  libcpu_src + libvio_src + libsdb_src,
  include_directories : inc,
  pic : true,
  install : true,
)

install_subdir('include/libcpu', install_dir : get_option('includedir'))
install_subdir('include/libsdb', install_dir : get_option('includedir'))
install_subdir('include/libvio', install_dir : get_option('includedir'))

anemo_dep = declare_dependency(
  link_with : libanemo,
  include_directories : inc,
)

executable('quick_start',   'src/examples/quick_start.cc',   dependencies : anemo_dep)
executable('riscv_minimal', 'src/examples/riscv_minimal.cc', dependencies : anemo_dep)
