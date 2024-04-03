#pragma once

#include "bus.h"
#include "processes_data_exchange.h"
#include "worker.h"
#include "worker_gc.h"

#include <map>
#include <vector>

class Dataplane {
public:
	void Init(bool useHugeMem);
	void Start();
	void Join();

private:
	Bus bus;
	common::processes_data_exchange::MainFileData processes_data;

	std::map<tCoreId, Worker*> workers;
	std::map<tCoreId, WorkerGC*> workers_gc;

	std::map<tSocketId, std::vector<tCoreId>> config_workers;
	std::map<tSocketId, std::vector<tCoreId>> config_workers_gc; 
};
