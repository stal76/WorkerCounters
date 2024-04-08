#pragma once

#include "pde.h"
#include <thread>

class Worker {
public:
	Worker(tCoreId core_id);
	static uint64_t FillMetadataWorkerCounters(common::pde::MetadataWorker* metadata);
	void SetBufferForCounters(void* buffer, const common::pde::MetadataWorker& metadata);
	void Start();
	void Join();

private:
	tCoreId core_id_;
	std::thread* main_trhead = nullptr;	// shared_ptr;
	void MainThread();

	// common::worker::stats::common stats;
	// common::worker::stats::port statsPorts[CONFIG_YADECAP_PORTS_SIZE];
	// uint64_t bursts[CONFIG_YADECAP_MBUFS_BURST_SIZE + 1];
	// uint64_t counters[YANET_CONFIG_COUNTERS_SIZE];
	// uint64_t aclCounters[YANET_CONFIG_ACL_COUNTERS_SIZE];

	uint64_t* counters;
	uint64_t* aclCounters;
	uint64_t* bursts;
	common::worker::stats::common* stats;
	common::worker::stats::port* statsPorts;
};
