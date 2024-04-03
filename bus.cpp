#include "bus.h"

void Bus::SetBuffers(uint64_t* buf_requests, uint64_t* buf_errors) {
	requests = buf_requests;
	errors = buf_errors;
}

void Bus::SomeWork() {
	for (uint32_t index = 0; index < static_cast<uint32_t>(common::idp::errorType::size); ++index) {
		errors[index] = 1 + index;
	}

	for (uint32_t index = 0; index < static_cast<uint32_t>(common::idp::requestType::size); ++index) {
		requests[index] = (1 + index) * (1 + index);
	}
}

