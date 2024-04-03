#include <iostream>
#include <unistd.h>

#include "common.h"
#include "dataplane.h"

common::log::LogPriority common::log::logPriority = common::log::TLOG_DEBUG;

void Dataplane::Init(bool useHugeMem) {
	Worker::FillMetadataWorkerCounters(&processes_data.metadata_workers, 0);
	WorkerGC::FillMetadataWorkerCounters(&processes_data.metadata_workers_gc, 0);

	config_workers[0] = {0, 3};
	config_workers[5] = {2, 7};
	config_workers_gc[0] = {8};
	config_workers_gc[5] = {11};

	for (const auto& iter : config_workers) {
		for (tCoreId core_id: iter.second) {
			processes_data.AddWorker(iter.first, core_id);
			workers[core_id] = new Worker(core_id);
		}
	}

	for (const auto& iter : config_workers_gc) {
		for (tCoreId core_id: iter.second) {
			processes_data.AddWorkerGC(iter.first, core_id);
			workers_gc[core_id] = new WorkerGC(core_id);
		}
	}	

	eResult result = processes_data.FinalizeDataplane(useHugeMem);
	if (result != eResult::success) {
		exit(1);
	}
	bus.SetBuffers(processes_data.GetStartBufferOtherCounters(processes_data.metadata_other_counters.start_bus_requests),
				   processes_data.GetStartBufferOtherCounters(processes_data.metadata_other_counters.start_bus_errors));
}

void Dataplane::Start() {
	for (auto [core_id, worker] : workers) {
		worker->SetBufferForCounters(processes_data.GetStartBufferForWorker(core_id), processes_data.metadata_workers);
		worker->Start();
	}

	for (auto [core_id, worker] : workers_gc) {
		worker->SetBufferForCounters(processes_data.GetStartBufferForWorkerGC(core_id), processes_data.metadata_workers_gc);
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
