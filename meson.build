project('WorkerCounters', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17', 'buildtype=release', 'werror=true'])

executable('dataplane',
           'bus.cpp',
           'dataplane.cpp',
           'worker.cpp',
           'worker_gc.cpp',
           'shared_memory.cpp',
           'processes_data_exchange.cpp',
           install : true)

executable('cli',
           'cli.cpp',
           'shared_memory.cpp',
           'processes_data_exchange.cpp',
           install : true)
