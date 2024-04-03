#pragma once

#include "processes_data_exchange.h"

class WorkerGC {
public:
	WorkerGC(tCoreId core_id);
	static uint64_t FillMetadataWorkerCounters(common::processes_data_exchange::MetadataWorkerGcCounters* metadata, uint64_t start_position);
	void SetBufferForCounters(void* buffer, const common::processes_data_exchange::MetadataWorkerGcCounters& metadata);
	void Start();
	void Join();

private:
	tCoreId core_id_;
	std::thread* main_trhead = nullptr;	// shared_ptr;
	void MainThread();

	// uint64_t counters[YANET_CONFIG_COUNTERS_SIZE];
	// common::worker_gc::stats_t stats;

	uint64_t* counters;
	common::worker_gc::stats_t* stats;
};
