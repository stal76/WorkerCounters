#pragma once

#include "pde.h"

class WorkerGC {
public:
	WorkerGC(tCoreId core_id);
	static uint64_t FillMetadataWorkerCounters(common::pde::MetadataWorkerGc* metadata);
	void SetBufferForCounters(void* buffer, const common::pde::MetadataWorkerGc& metadata);
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
