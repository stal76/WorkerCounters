#include <iostream>
#include <thread>

#include "worker_gc.h"

WorkerGC::WorkerGC(tCoreId core_id) : core_id_(core_id) {
}

uint64_t WorkerGC::FillMetadataWorkerCounters(common::processes_data_exchange::MetadataWorkerGcCounters* metadata, uint64_t start_position) {
	metadata->start_counters = start_position;
    metadata->start_stats = common::processes_data_exchange::BufferWriter::AllignToSizeCacheLine(metadata->start_counters + YANET_CONFIG_COUNTERS_SIZE * sizeof(uint64_t));
	uint64_t start_next = common::processes_data_exchange::BufferWriter::AllignToSizeCacheLine(metadata->start_stats + sizeof(common::worker_gc::stats_t)); // ?
	metadata->total_size = start_next - start_position;
	metadata->index_counters = metadata->start_counters / 8;

	// stats
	common::worker_gc::stats_t fake_stats;
	std::map<std::string, void*> counters_stats;
	counters_stats["brokenPackets"] = &fake_stats.broken_packets;
	counters_stats["dropPackets"] = &fake_stats.drop_packets;
	counters_stats["ring_to_slowworker_packets"] = &fake_stats.ring_to_slowworker_packets;
	counters_stats["ring_to_slowworker_drops"] = &fake_stats.ring_to_slowworker_drops;
	counters_stats["fwsync_multicast_egress_packets"] = &fake_stats.fwsync_multicast_egress_packets;
	counters_stats["fwsync_multicast_egress_drops"] = &fake_stats.fwsync_multicast_egress_drops;
	counters_stats["fwsync_unicast_egress_packets"] = &fake_stats.fwsync_unicast_egress_packets;
	counters_stats["fwsync_unicast_egress_drops"] = &fake_stats.fwsync_unicast_egress_drops;
	counters_stats["drop_samples"] = &fake_stats.drop_samples;
	counters_stats["balancer_state_insert_failed"] = &fake_stats.balancer_state_insert_failed;
	counters_stats["balancer_state_insert_done"] = &fake_stats.balancer_state_insert_done;

	metadata->AddCounters(counters_stats, metadata->start_stats, &fake_stats);

	return start_next;
}

void WorkerGC::SetBufferForCounters(void* buffer, const common::processes_data_exchange::MetadataWorkerGcCounters& metadata) {
	// std::cout << "\tSet buffers worker_gc " << core_id_ << ": " << buffer << "\n";
	counters = static_cast<uint64_t*>(common::processes_data_exchange::PtrAdd(buffer, metadata.start_counters));
	stats = static_cast<common::worker_gc::stats_t*>(common::processes_data_exchange::PtrAdd(buffer, metadata.start_stats));
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
