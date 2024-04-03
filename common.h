#pragma once

#include <ctime>
#include <stdint.h>

namespace common::log
{
enum LogPriority
{
	TLOG_EMERG = 0 /* "EMERG" */,
	TLOG_ALERT = 1 /* "ALERT" */,
	TLOG_CRIT = 2 /* "CRITICAL_INFO" */,
	TLOG_ERR = 3 /* "ERROR" */,
	TLOG_WARNING = 4 /* "WARNING" */,
	TLOG_NOTICE = 5 /* "NOTICE" */,
	TLOG_INFO = 6 /* "INFO" */,
	TLOG_DEBUG = 7 /* "DEBUG" */,
	TLOG_RESOURCES = 8 /* "RESOURCES" */
};
extern LogPriority logPriority;
} // common::log

#define YANET_LOG_(name, level, msg, ...)                                                                                          \
	do                                                                                                                             \
		if (common::log::logPriority >= common::log::TLOG_##level)                                                             \
		{                                                                                                                      \
			timespec ts;                                                                                                   \
			timespec_get(&ts, TIME_UTC);                                                                                   \
			fprintf(stdout, "[" name "] %lu.%06lu %s:%d: " msg, ts.tv_sec, ts.tv_nsec / 1000, __FILE__, __LINE__, __VA_ARGS__); \
			fflush(stdout);                                                                                                \
		}                                                                                                                      \
	while (0)
#define YANET_LOG(level, msg, ...) YANET_LOG_(#level, level, msg, __VA_ARGS__)

#define YANET_LOG_ERROR(msg, ...) YANET_LOG_("ERROR", ERR, msg, __VA_ARGS__)
#define YADECAP_LOG_ERROR YANET_LOG_ERROR

enum class result_e : uint32_t
{
	success,
	errorAllocatingMemory,
	errorAllocatingKernelInterface,
	errorInitEal,
	errorInitBarrier,
	errorInitMempool,
	errorInitEthernetDevice,
	errorInitQueue,
	errorInitRing,
	errorCreateThread,
	errorOpenFile,
	errorSocket,
	errorBind,
	errorListen,
	errorConnect,
	invalidId,
	invalidCoreId,
	invalidSocketId,
	invalidConfigurationFile,
	invalidPrefix,
	invalidValueId,
	invalidPortId,
	invalidPortsCount,
	invalidInterfaceName,
	invalidCoresCount,
	invalidCount,
	invalidCounterId,
	invalidWorkerPortId,
	invalidNeighbor,
	invalidNexthop,
	invalidLogicalPortId,
	invalidVlanId,
	invalidFlow,
	invalidDecapId,
	invalidInterfaceId,
	invalidNat64statefulId,
	invalidNat64statelessId,
	invalidNat64statelessTranslationId,
	invalidAclId,
	invalidType,
	invalidArguments,
	invalidPhysicalPortName,
	invalidRing,
	isFull,
	isEmpty,
	unsupported,
	unsupportedDevice,
	alreadyExist,
	invalidMulticastIPv6Address,
	dataplaneIsBroken,
	invalidJson,
	missingRequiredOption,
	invalidTun64Id,
	errorInitSharedMemory,
};

using eResult = result_e;
