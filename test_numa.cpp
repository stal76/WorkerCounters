#include <fstream>
#include <iostream>
#include <map>
#include <vector>

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

#include <numa.h>
#include <numaif.h>
#include <utmpx.h>

// Что пытаемся решить - выделить в Shared Memory память на нужной Numa node.
//
// В Windows есть функция CreateFileMappingNumaW (memoryapi.h):
//     https://learn.microsoft.com/ru-ru/windows/win32/api/memoryapi/nf-memoryapi-createfilemappingnumaw?redirectedfrom=MSDN
//
// Для Linux сравнивается 2 подхода:
//    1. Перед выделением памяти осуществляется вызов numa_run_on_node
//    2. Использовать set_mempolicy, этот подход используется в DPDK (lib/eal/linux/eal_memory.c)
//       Можно еще использовать mbind (наверное)
// Подробнее описано в комментариях в функции TestAllocation
// Оба подхода дали одинаковый итоговый результат
//
// По умолчанию нет гарантий, что память не переедет на другую Numa node, но на рабочих серверах при загрузке ядра 
// устанавливается параметр: 
//	numa_balancing=disable
//
// В документации ядра написано следующее:
// numa_balancing
// ==============
//
// Enables/disables and configures automatic page fault based NUMA memory balancing.  Memory is moved automatically
// to nodes that access it often. The value to set can be the result of ORing the following:
//
// = =================================
// 0 NUMA_BALANCING_DISABLED
// 1 NUMA_BALANCING_NORMAL
// 2 NUMA_BALANCING_MEMORY_TIERING
// = =================================


std::map<int, std::vector<int>> GetCpuByNodes();
void SayCurrentCpu();
std::ostream& operator<< (std::ostream& out, const struct bitmask* mask);
std::ostream& operator<< (std::ostream& out, const std::vector<int>& data);
void* CreateBufferInSharedMemory(const char* filename, u_int64_t size, bool useHugeMem);
void ShowMainInfoAboutNuma();

// По указателю хотим определить Numa node на которой выделена эта память
// Выводит из /proc/self/numa_maps все строки которые начинаются с hex адреса ptr
//
// В lib/eal/linux/eal_memory.c есть интересная функция rte_mem_virt2phy - 
// "Get physical address of any mapped virtual address in the current process."
void FindMemoryNumaInfo(void* ptr) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%p", ptr);
    std::string address = (buf + 2);
    
    std::ifstream file("/proc/self/numa_maps");
    std::string str;
    while (std::getline(file, str)) {
        if (str.size() >= address.size() && str.substr(0, address.size()) == address) {
            std::cout << "                           " << str << "\n";
        }
    }
}

// Основная функция которая запускаетс тесты, значение параметра kind:
//    1 - Используется numa_set_preferred
//    2 - Используется numa_run_on_node
void TestAllocation(size_t size, bool useHugeMem, int kind) {
    std::cout << "\nRun Test Allocation " << kind << "\n";

    // Для kind = 1 - запомним текущие настройки памяти, при выходе восстановим какие были:
    // - numa_allocate_nodemask - выделяет память под структуру
    // - get_mempolicy - заполняет ее
    struct bitmask* oldmask = nullptr;
    int oldpolicy;
    if (kind == 1) {
        oldmask = numa_allocate_nodemask();
        if (get_mempolicy(&oldpolicy, oldmask->maskp, oldmask->size + 1, 0, 0) < 0) {
            std::cout << "Error get_mempolicy\n";
            oldpolicy = MPOL_DEFAULT;
        }
        std::cout << "oldpolicy = " << oldpolicy << "\n";
    }
    
    auto cpu_by_nodes = GetCpuByNodes();
    for (const auto& iter : cpu_by_nodes) {
        // Переберем все Numa node
        if (kind == 1) {
            std::cout << "Set preferred node: " << iter.first << "\n";
            // Делает вызов в ядре:
            // node=0: set_mempolicy(MPOL_PREFERRED, [0x0000000000000001], 65) = 0
            // node=1: set_mempolicy(MPOL_PREFERRED, [0x0000000000000002], 65) = 0
            numa_set_preferred(iter.first);
        } else {
            std::cout << "numa_run_on_node: " << iter.first << "\n";
            // Делает вызовы в ядре:
            // node=0: sched_setaffinity(0, 32, [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13]) = 0
            // node=1: sched_setaffinity(0, 32, [14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27]) = 0
            int res = numa_run_on_node(iter.first);
            if (res < 0) {
                perror("Error numa_run_on_node:");
            }
        }
        SayCurrentCpu();

        // Выделение памяти через numa_alloc_onnode:
        //    mmap(NULL, 1048576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0) = 0x7f8231cef000
        //    mbind(0x7f8231cef000, 1048576, MPOL_BIND, [0x0000000000000001], 65, 0) = 0
        void* buf_numa_alloc_onnode = numa_alloc_onnode(size, iter.first);

        // Выделение памяти через malloc:
        //    mmap(NULL, 1052672, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f8230773000
        void* buf_malloc = malloc(size);

        // Выделение памяти в Shared Memory
        std::string filename = "/test" + std::to_string(kind) + "_numa_" + std::to_string(iter.first);
        void* buf_mmap = CreateBufferInSharedMemory(filename.c_str(), size, useHugeMem);
        
        // Вывод результатов, для каждого буфера ищется информация в /proc/self/numa_maps
        std::cout << "  buf_numa_alloc_onnode: " << buf_numa_alloc_onnode << "\n";
        FindMemoryNumaInfo(buf_numa_alloc_onnode);
        std::cout << "  buf_malloc:            " << buf_malloc << "\n";
        FindMemoryNumaInfo(buf_malloc);
        FindMemoryNumaInfo((char*)buf_malloc - 16);
        std::cout << "  buf_mmap:              " << buf_mmap << "\n";
        FindMemoryNumaInfo(buf_mmap);
    }

    // Восстановим предыдущие настройки памяти
    if (kind == 1) {
        // В зависимости от того, возникали ли проблемы, вызывается set_mempolicy или numa_set_localalloc
        if (oldpolicy == MPOL_DEFAULT) {
            numa_set_localalloc();
        } else if (set_mempolicy(oldpolicy, oldmask->maskp, oldmask->size + 1) < 0) {
            std::cout << "Error get_mempolicy\n";
            numa_set_localalloc();
        }
        numa_free_cpumask(oldmask);
    }


    /*
        Вывод результатов тестов. Видно, что память выделяется на нужных Numa nodes.
        Для numa_alloc_onnode - строка "bind:0" или "bin:1"
        Для двух других способов в строках встречается "N0=1" или "N1=1"

        Run Test Allocation 1
        oldpolicy = 0
        Set preferred node: 0
        current cpu: 2, node: 0
        buf_numa_alloc_onnode: 0x7f8231cef000
                                7f8231cef000 bind:0
        buf_malloc:            0x7f8230773010
                                7f8230773000 prefer:0 anon=1 dirty=1 N0=1 kernelpagesize_kB=4
        buf_mmap:              0x7f8230673000
                                7f8230673000 prefer:0 file=/dev/shm/test1_numa_0 dirty=256 N0=256 kernelpagesize_kB=4
        Set preferred node: 1
        current cpu: 2, node: 0
        buf_numa_alloc_onnode: 0x7f8230573000
                                7f8230573000 bind:1
        buf_malloc:            0x7f8230472010
                                7f8230472000 prefer:1 anon=1 dirty=1 N1=1 kernelpagesize_kB=4
        buf_mmap:              0x7f8230372000
                                7f8230372000 prefer:1 file=/dev/shm/test1_numa_1 dirty=256 N1=256 kernelpagesize_kB=4

        Run Test Allocation 2
        numa_run_on_node: 0
        current cpu: 2, node: 0
        buf_numa_alloc_onnode: 0x7f8230272000
                                7f8230272000 bind:0
        buf_malloc:            0x7f8230171010
                                7f8230171000 default anon=1 dirty=1 N0=1 kernelpagesize_kB=4
        buf_mmap:              0x7f8230071000
                                7f8230071000 default file=/dev/shm/test2_numa_0 dirty=256 active=0 N0=256 kernelpagesize_kB=4
        numa_run_on_node: 1
        current cpu: 14, node: 1
        buf_numa_alloc_onnode: 0x7f822ff71000
                                7f822ff71000 bind:1
        buf_malloc:            0x7f822fe70010
                                7f822fe70000 default anon=1 dirty=1 N1=1 kernelpagesize_kB=4
        buf_mmap:              0x7f822fd70000
                                7f822fd70000 default file=/dev/shm/test2_numa_1 dirty=256 active=0 N1=256 kernelpagesize_kB=4
    */
}

int main() {
    std::cout << "Start test numa, pid = " << getpid() << "\n";
    SayCurrentCpu();

    ShowMainInfoAboutNuma();

    size_t size = 1024 * 1024;
    bool useHugeMem = false;

    TestAllocation(size, useHugeMem, 1);
    TestAllocation(size, useHugeMem, 2);

    int stop;
    std::cin >> stop;
    return 0;
}

// Вывести информация о Numa
void ShowMainInfoAboutNuma() {
    // Выводит примерно такое:
    //
    /*
        Main info about NUMA
        numa_available: 0
        numa_max_possible_node: 63
        numa_num_possible_nodes: 64
        numa_max_node: 1
        numa_num_configured_nodes: 2
        numa_num_task_cpus: 28
        numa_num_task_nodes: 2
        numa_get_mems_allowed: 2 [0,1]
        numa_all_nodes_ptr: 2 [0,1]
        numa_no_nodes_ptr: 0 []
        numa_all_cpus_ptr: 28 [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27]
        numa_get_run_node_mask: 2 [0,1]
        cpu by node:
            0: 14 [0,1,2,3,4,5,6,7,8,9,10,11,12,13]
            1: 14 [14,15,16,17,18,19,20,21,22,23,24,25,26,27]
    */
    // Многи разных функций про Numa тут:
    //  https://man7.org/linux/man-pages/man3/numa.3.html

    std::cout << "\nMain info about NUMA\n";

    int available = numa_available();
    std::cout << "numa_available: " << available << "\n";

    std::cout << "numa_max_possible_node: " << numa_max_possible_node() << "\n";
    std::cout << "numa_num_possible_nodes: " << numa_num_possible_nodes() << "\n";
    std::cout << "numa_max_node: " << numa_max_node() << "\n";
    std::cout << "numa_num_configured_nodes: " << numa_num_configured_nodes() << "\n";
    std::cout << "numa_num_task_cpus: " << numa_num_task_cpus() << "\n";
    std::cout << "numa_num_task_nodes: " << numa_num_task_nodes() << "\n";

    std::cout << "numa_get_mems_allowed: " << numa_get_mems_allowed() << "\n";
    std::cout << "numa_all_nodes_ptr: " <<numa_all_nodes_ptr << "\n";
    std::cout << "numa_no_nodes_ptr: " << numa_no_nodes_ptr << "\n";
    std::cout << "numa_all_cpus_ptr: " << numa_all_cpus_ptr << "\n";
    std::cout << "numa_get_run_node_mask: " << numa_get_run_node_mask() << "\n";

    std::cout << "cpu by node:\n";
    for (const auto& iter : GetCpuByNodes()) {
        std::cout << "\t" << iter.first << ": " << iter.second << "\n";
    }
    std::cout << "\n\n";

    /*
        Как происходит получение списка cpu по Numa nodes (что показал strace).
        В каталоге /sys/devices/system/node/ ищутся все каталоги с нодами, в моем случае их там 2,
        в каждом каталоге читается содержимое файла cpumap, там одно hex число с битовой маской cpu.
        - /sys/devices/system/node/node1/cpumap: 0003fff - биты 0-13 
        - /sys/devices/system/node/node2/cpumap: fffc000 - биты 14-27
        Есть также файлы cpulist с содержимым: "0-13" и "14-17" соответственно
    */
}

// ----------------------------------------------------------------------------
// ДАЛЬШЕ ИДУТ ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ----------------------------------------------------------------------------

std::vector<int> BitmapToVector(const struct bitmask* mask) {
    std::vector<int> result;
    for (unsigned long i = 0; i < mask->size; i++) {
        if (((mask->maskp[i / (8 * sizeof(*mask->maskp))] >> (i % (8 * sizeof(*mask->maskp)))) & 1) == 1) {
            result.push_back(i);
        }
    }
    return result;
}

std::ostream& operator<< (std::ostream& out, const std::vector<int>& data) {
    out << data.size() << " [";
    bool first = true;
    for (int value : data) {
        if (!first) {
            out << ",";
        } else {
            first = false;
        }
        out << value;
    }
    out << "]";
    return out;
}

std::ostream& operator<< (std::ostream& out, const struct bitmask* mask) {
    out << BitmapToVector(mask);
    return out;
}

std::map<int, std::vector<int>> GetCpuByNodes() {
    std::map<int, std::vector<int>> cpus_by_nodes;
    std::vector<int> all_cpus = BitmapToVector(numa_all_cpus_ptr);
    for (int cpu_id : all_cpus) {
        cpus_by_nodes[numa_node_of_cpu(cpu_id)].push_back(cpu_id);
    }
    return cpus_by_nodes;
}

void SayCurrentCpu() {
    int cpu = sched_getcpu();
    std::cout << "current cpu: " << cpu << ", node: " << numa_node_of_cpu(cpu) << "\n";
}

// Копия функции из shared_memory.cpp
#define YADECAP_LOG_ERROR printf
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

