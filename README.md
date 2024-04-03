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

## Файлы проекта

На основной проект будут переноситься только:
- processes_data_exchange.cpp
- processes_data_exchange.h
- shared_memory.cpp
- shared_memory.h
Все остальные файлы только эммулируют объекты на рабочем проекте и выполняют тестирование.
