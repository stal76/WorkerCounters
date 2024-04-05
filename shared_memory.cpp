#include "common.h"
#include "shared_memory.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

// - В случае ошибок происходит логирование через YADECAP_LOG_ERROR и возвращается nullptr
// - Настройки shared memory в системе:
//		/proc/sys/kernel/shmmax - максимальный размер, у меня стоит значение близкое к 2^64
//		/proc/sys/kernel/shmmni - максимальное количество сегментов, у меня стоит 4096
//		/proc/sys/kernel/shmall - максимальное количество страниц, у меня значение равное shmmax
// - Есть настройка разрешающая использование HugeTLB, по умолчанию выключена. Если попробовать
//   сделать mmap с hugeMem при выключенной возможности в ядре, то вернет ошибку. Также,
//   пользователь должен быть добавлен в нужную группу (?).
// - Если процесс "писатель" завершился, то "читатель" может продолжить читать. Даже если
//   "писатель" включится при работающем читателе, то читатель начнет видеть изменения вносимые
//   новым "писателем".
// - Может включиться 2 "писателя", одновременно писать (не очень хорошо).
// - Файлы не закрываем, в принципе после mmap можно закрыть, дальше все продолжает работать.
//   unmap не делаем, не очень красиво, но все равно это должно происходить только при завершении
//   работы программы.
// - Чтобы mmap выделил память на нужной Numa node перед вызовом CreateBufferInSharedMemory надо
//   запускать numa_run_on_node или через set_mempolicy. Подробнее об этом в test_numa.cpp

// Хочется еще протестировать:
// - все работает с размером файла > 4GB
// - протестировать hugeMem, в том числе проверить в свойствах процесса в/proc информацию о huge
// - протестировать скорость работы

// Документация по системным вызовам:
// - https://man7.org/linux/man-pages/man3/shm_open.3.html
// - https://man7.org/linux/man-pages/man2/mmap.2.html
// - https://man7.org/linux/man-pages/man3/fstat.3p.html

static_assert(sizeof(__off_t) == 8, "Sizeof(__off_t) != 8");

void* CreateBufferInSharedMemory(const char* filename, u_int64_t size, bool useHugeMem) {	
	// - В документации написано, что имя надо задавать в фоормате "/somename".
	// - Файлы создаются в /dev/shm, их можно открывать на просмотр, например, mc.
	// - Создаем с флагами O_RDWR | O_CREAT.
	// - Используем shm_open, есть вариант использовать shmget, но второй вариант использует
	//   числовой ключ и он пришел из SystemV, первый линуксовый
	int flags_open = O_RDWR | O_CREAT;
	int mode = 0644;
	int fd = shm_open(filename, O_RDWR | O_CREAT, 0644);
	if (fd == -1) {
		YADECAP_LOG_ERROR("shm_open(%s, %d, %o): %s\n", filename, flags_open, mode, strerror(errno));
		return nullptr;
	}

	// Устанавливаем размер
	int res_trunc = ftruncate(fd, size);
	if (res_trunc < 0) {
		YADECAP_LOG_ERROR("filename=%s, ftruncate(%d, %lu): %s\n", filename, fd, size, strerror(errno));
		return nullptr;
	}

	// - При использовании HugeMEM надо включать флаг MAP_HUGETLB, подробнее про это можно почитать
	//   в документации ядра Linux: Documentation/admin-guide/mm/hugetlbpage.rst. Требуется
	//   включить возможность использования в ядре. Возможно надо подключить один из дополнительных
	//   флагов MAP_HUGE_2MB, MAP_HUGE_1GB - если на системе используются разные размеры для hugetlb
	//   page sizes
	int prot = PROT_READ | PROT_WRITE;
	int flags_mmap = (useHugeMem ? MAP_SHARED | MAP_HUGETLB : MAP_SHARED);
	void* addr = mmap(NULL, size, prot, flags_mmap, fd, 0);
	if (addr == MAP_FAILED) {
		// Ошибка возникает при попытке использовать HugeMem когда не включена это возможность в ядре
		YADECAP_LOG_ERROR("filename=%s, mmap(NULL, %lu, %d, %d, %d, 0)\n", filename, size, prot, flags_mmap, fd);
		return nullptr;
	}

	// Очистим память, автоматическая очистка происходит вроде только при включенном флаге MAP_ANONYMOUS
	std::memset(addr, 0, size);

	// Можно закрыть файловый дескриптор, после закрытия все работает также
	// close(fd);
	
	return addr;
}

void* OpenBufferInSharedMemory(const char* filename, bool forWriting, bool useHugeMem, u_int64_t* size) {
	int flags_open = (forWriting ? O_RDWR : O_RDONLY);
	int mode = 0644;
	int fd = shm_open(filename, flags_open, 0644);
	if (fd == -1) {
		YADECAP_LOG_ERROR("shm_open(%s, %d, %o): %s\n", filename, flags_open, mode, strerror(errno));
		return nullptr;
	}

	// - Получим размер файла, используем fstat.
	//   Здесь утверждается, что через lseek может быть UB:
	//   https://stackoverflow.com/questions/32795449/check-posix-shared-memory-object-size
	struct stat buffer;
	int status = fstat(fd, &buffer);
	if (status != 0) {
		YADECAP_LOG_ERROR("filename=%s, fstat(%d, &buffer): %s\n", filename, fd, strerror(errno));
		return nullptr;
	}
	*size = buffer.st_size;

	int prot = (forWriting ? PROT_READ | PROT_WRITE : PROT_READ);
	int flags_mmap = (useHugeMem ? MAP_SHARED | MAP_HUGETLB : MAP_SHARED);
	void* addr = mmap(NULL, *size, prot, flags_mmap, fd, 0);
	if (addr == MAP_FAILED) {
		// Ошибка возникает при попытке использовать HugeMem когда не включена это возможность в ядре
		YADECAP_LOG_ERROR("filename=%s, mmap(NULL, %lu, %d, %d, %d, 0)\n", filename, *size, prot, flags_mmap, fd);
		return nullptr;
	}

	// Можно закрыть файловый дескриптор, после закрытия все работает также
	// close(fd);
	
	return addr;
}
