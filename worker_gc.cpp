#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>

#include "worker_gc.h"

WorkerGC::WorkerGC(tCoreId core_id) : core_id_(core_id) {
}

uint64_t WorkerGC::FillMetadataWorkerCounters(common::pde::MetadataWorkerGc* metadata) {
	metadata->start_counters = 0;
    metadata->start_stats = common::pde::AllignToSizeCacheLine(metadata->start_counters + YANET_CONFIG_COUNTERS_SIZE * sizeof(uint64_t));
	uint64_t start_next = common::pde::AllignToSizeCacheLine(metadata->start_stats + sizeof(common::worker_gc::stats_t)); // ?
	metadata->total_size = start_next;
	metadata->index_counters = metadata->start_counters / 8;

	// stats
	static_assert(std::is_trivially_destructible<common::worker_gc::stats_t>::value, "invalid struct destructible");
	std::map<std::string, uint64_t> counters_stats;
	counters_stats["brokenPackets"] = offsetof(common::worker_gc::stats_t, broken_packets);
	counters_stats["dropPackets"] = offsetof(common::worker_gc::stats_t, drop_packets);
	counters_stats["ring_to_slowworker_packets"] = offsetof(common::worker_gc::stats_t, ring_to_slowworker_packets);
	counters_stats["ring_to_slowworker_drops"] = offsetof(common::worker_gc::stats_t, ring_to_slowworker_drops);
	counters_stats["fwsync_multicast_egress_packets"] = offsetof(common::worker_gc::stats_t, fwsync_multicast_egress_packets);
	counters_stats["fwsync_multicast_egress_drops"] = offsetof(common::worker_gc::stats_t, fwsync_multicast_egress_drops);
	counters_stats["fwsync_unicast_egress_packets"] = offsetof(common::worker_gc::stats_t, fwsync_unicast_egress_packets);
	counters_stats["fwsync_unicast_egress_drops"] = offsetof(common::worker_gc::stats_t, fwsync_unicast_egress_drops);
	counters_stats["drop_samples"] = offsetof(common::worker_gc::stats_t, drop_samples);
	counters_stats["balancer_state_insert_failed"] = offsetof(common::worker_gc::stats_t, balancer_state_insert_failed);
	counters_stats["balancer_state_insert_done"] = offsetof(common::worker_gc::stats_t, balancer_state_insert_done);

	for (const auto& iter : counters_stats) {
	    metadata->counter_positions[iter.first] = (metadata->start_stats + iter.second) / sizeof(uint64_t);
    }

	return start_next;
}

void WorkerGC::SetBufferForCounters(void* buffer, const common::pde::MetadataWorkerGc& metadata) {
	// std::cout << "\tSet buffers worker_gc " << core_id_ << ": " << buffer << "\n";
	counters = static_cast<uint64_t*>(common::pde::PtrAdd(buffer, metadata.start_counters));
	stats = static_cast<common::worker_gc::stats_t*>(common::pde::PtrAdd(buffer, metadata.start_stats));
}

void WorkerGC::Start() {
	main_trhead = new std::thread([this]() {
		MainThread();
	});
	std::cout << "Start worker_gc " << core_id_ << " thread\n";
}

void WorkerGC::Join() {
	if (main_trhead != nullptr) {
		main_trhead->join();
		delete main_trhead;
	}
}

void WorkerGC::MainThread() {
	uint64_t prefix_core = (((uint64_t)core_id_ << 8) + 0xff) << 48;
	stats->ring_to_slowworker_drops = prefix_core;
	counters[11] = prefix_core;
	while (true) {
		stats->ring_to_slowworker_drops++;
		counters[11]++;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
