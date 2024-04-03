#pragma once

#include "type.h"

class Bus {
public:
	void SetBuffers(uint64_t* buf_requests, uint64_t* buf_errors);

	void SomeWork();

private:
	// uint64_t requests[(uint32_t)common::idp::requestType::size];
	// uint64_t errors[(uint32_t)common::idp::errorType::size];

	uint64_t* requests;
	uint64_t* errors;
};
