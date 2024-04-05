# WorkerCouunters
Проект для проверки работы counters при переходе на использование Shared Memory

## Сборка
```
meson compile -C build
```

## Запуск тестов

Dataplane:
```
build\dataplane
```

Cli:
```
build\cli
```

Test Numa:
```
build\test_numa
```

## Файлы проекта

На основной проект будут переноситься только:

- processes_data_exchange.cpp
- processes_data_exchange.h
- shared_memory.cpp
- shared_memory.h

В shared_memory.cpp и test_numa.cpp - много подробных полезных комментариев.

Все остальные файлы только эммулируют объекты на рабочем проекте и выполняют тестирование.
