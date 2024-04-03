#include <iostream>
#include <vector>
#include <thread>
#include <unistd.h>

#include "common.h"
#include "processes_data_exchange.h"

common::log::LogPriority common::log::logPriority = common::log::TLOG_DEBUG;

void TestBusStat(uint64_t* buffer_errors, uint64_t* buffer_stats) {
    // In test Dataplane save: errors[i] = i + 1; stats[i] = (i + 1)^2

    for (int index = 0; index < (int)common::idp::errorType::size; ++index) {
        uint64_t value = buffer_errors[index];
        uint64_t expected = index + 1;
        if (value != expected) {
            std::cerr << "Error bus stat errors, at " << index << " value: " << value << ", expected: " << expected << "\n";
            exit(1);
        }
    }
    std::cout << "Check bus stat errors: OK!\n";

    for (int index = 0; index < (int)common::idp::requestType::size; ++index) {
        uint64_t value = buffer_stats[index];
        uint64_t expected = (index + 1) * (index + 1);
        if (value != expected) {
            std::cerr << "Error bus stat requests, at " << index << " value: " << value << ", expected: " << expected << "\n";
            exit(1);
        }
    }
    std::cout << "Check bus stat requests: OK!\n";
}

void ShowWorkerInfo(tCoreId core_id, uint8_t second_byte, const std::map<std::string, uint64_t>& counters) {
    uint64_t min_value = -1;
    uint64_t max_value = 0;
    for (auto [name, value] : counters) {
        // For each value check : higher byte equal core_id, second byte equal second_byte
        uint8_t core_id_get = value >> 56;
        uint8_t second_get = uint8_t(value >> 48);
        if (core_id != core_id_get || second_byte != second_get) {
            std::cerr << "Bad prefix for '" << name << "', core_id: " << core_id << ", " << int(core_id_get)
                      << ", prefix: " << int(second_byte) << ", " << int(second_get) << "\n";
            exit(1);
        }
        value = (value << 16) >> 16;
        min_value = std::min(min_value, value);
        max_value = std::max(max_value, value);
    }

    std::cout << core_id << ": " << min_value << "," << (max_value - min_value) << "  ";
}

int main() {
	std::cout << "Start cli, pid = " << getpid() << "\n";

    bool useHugeMem = false;
    common::processes_data_exchange::MainFileData processes_data;
    eResult result = processes_data.ReadFromDataplane(useHugeMem, true);
    if (result != eResult::success) {
        exit(1);
    }

    TestBusStat(processes_data.GetStartBufferOtherCounters(processes_data.metadata_other_counters.start_bus_errors),
                processes_data.GetStartBufferOtherCounters(processes_data.metadata_other_counters.start_bus_requests));

    uint64_t pos_in_cnts = processes_data.metadata_workers.counter_positions["balancer_icmp_generated_echo_reply_ipv6"];
    uint64_t pos_in_stats = processes_data.metadata_workers.counter_positions["decap_fragments"];
    uint64_t gc_pos_in_stats = processes_data.metadata_workers_gc.counter_positions["ring_to_slowworker_drops"];

    while (true) {
        std::cout << "\r";
        for (const auto& iter : processes_data.workers.workers) {
            uint64_t* buffer = (uint64_t*)processes_data.GetStartBufferForWorker(iter.first);

            // get by names - "named" counter and value from structure stats
            uint64_t from_counters = buffer[processes_data.metadata_workers.index_counters + pos_in_cnts];
            uint64_t from_stats = buffer[processes_data.metadata_workers.index_counters + pos_in_stats];

            // get from acl counter, in test check only at index 17
            uint64_t from_acl = buffer[processes_data.metadata_workers.index_acl_counters + 17];
            // get from bursts, in test check only at index 7
            uint64_t from_bursts = buffer[processes_data.metadata_workers.index_bursts + 7];
            // get from stats_ports, in test check only physicalPort_egress_drops in last element of array
            uint64_t from_stats_ports = buffer[processes_data.metadata_workers.index_stats_ports + 2 * CONFIG_YADECAP_PORTS_SIZE - 2];

            ShowWorkerInfo(iter.first, 0xfe,
                           {{"counters", from_counters}, {"stats", from_stats}, {"acl", from_acl},
                            {"bursts", from_bursts}, {"stats_ports", from_stats_ports}});
        }

        for (const auto& iter : processes_data.workers_gc.workers) {
            uint64_t* buffer = (uint64_t*)processes_data.GetStartBufferForWorkerGC(iter.first);

            uint64_t from_counters = buffer[processes_data.metadata_workers_gc.index_counters + 11];
            uint64_t from_stats = buffer[processes_data.metadata_workers_gc.index_counters + gc_pos_in_stats];

            ShowWorkerInfo(iter.first, 0xff, {{"gc_counters", from_counters}, {"gc_stats", from_stats}});
        }
        
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}
