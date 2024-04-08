#include <iostream>
#include <unistd.h>

#include "dataplane.h"

common::log::LogPriority common::log::logPriority = common::log::TLOG_DEBUG;

void Dataplane::Init(bool useHugeMem) {
	config_workers[0] = {0, 3};
	config_workers[1] = {2, 7};
	config_workers_gc[0] = {8};
	config_workers_gc[1] = {11};

	std::map<tCoreId, tSocketId> workers_to_sockets, workers_gc_to_sockets;

	for (const auto& iter : config_workers) {
		for (tCoreId core_id: iter.second) {
			workers_to_sockets[core_id] = iter.first;
			workers[core_id] = new Worker(core_id);
		}
	}

	for (const auto& iter : config_workers_gc) {
		for (tCoreId core_id: iter.second) {
			workers_gc_to_sockets[core_id] = iter.first;
			workers_gc[core_id] = new WorkerGC(core_id);
		}
	}	

	Worker::FillMetadataWorkerCounters(&processes_data.metadata_workers);
	WorkerGC::FillMetadataWorkerCounters(&processes_data.metadata_workers_gc);

	eResult result = processes_data.BuildFromDataPlane(workers_to_sockets, workers_gc_to_sockets, useHugeMem);
	if (result != eResult::success) {
		exit(1);
	}
	bus.SetBuffers(processes_data.BufferCommonCounters(processes_data.common_counters.start_bus_requests),
				   processes_data.BufferCommonCounters(processes_data.common_counters.start_bus_errors));
}

void Dataplane::Start() {
	for (auto [core_id, worker] : workers) {
		worker->SetBufferForCounters(processes_data.common_data.BufferWorker(core_id), processes_data.metadata_workers);
		worker->Start();
	}

	for (auto [core_id, worker] : workers_gc) {
		worker->SetBufferForCounters(processes_data.common_data.BufferWorkerGc(core_id), processes_data.metadata_workers_gc);
		worker->Start();
	}

	bus.SomeWork();
}

void Dataplane::Join() {
	for (auto [_, worker] : workers) {
		worker->Join();
	}

	for (auto [_, worker] : workers_gc) {
		worker->Join();
	}
}

int main() {
	bool useHugeMem = false;
	std::cout << "Start dataplane, pid = " << getpid() << "\n";

	Dataplane dataplane;
	dataplane.Init(useHugeMem);
	dataplane.Start();
	dataplane.Join();

	return 0;
}
