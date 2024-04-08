project('WorkerCounters', 'cpp',
  version : '0.1',
  default_options : ['warning_level=2', 'cpp_std=c++17', 'buildtype=release', 'werror=true', 'b_lto=true'])

executable('dataplane',
           'bus.cpp',
           'dataplane.cpp',
           'worker.cpp',
           'worker_gc.cpp',
           dependencies: dependency('numa'),
           override_options: ['cpp_eh=none', 'cpp_rtti=false'],
           install : true)

executable('cli',
           'cli.cpp',
           dependencies: dependency('numa'),
           install : true)

executable('test_numa',
           'test_numa.cpp',
           dependencies: dependency('numa'),
           install : true)
