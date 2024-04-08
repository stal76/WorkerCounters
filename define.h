#pragma once

#include <cstdio>
#include <ctime>
#include <map>
#include <optional>
#include <set>
#include <vector>

#include <inttypes.h>

#define YANET_UNUSED [[maybe_unused]]

#define YANET_LOG_PRINT(msg, args...) fprintf(stdout, msg, ##args)

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

#define YANET_LOG_(name, level, msg, args...)                                                                                          \
	do                                                                                                                             \
		if (common::log::logPriority >= common::log::TLOG_##level)                                                             \
		{                                                                                                                      \
			timespec ts;                                                                                                   \
			timespec_get(&ts, TIME_UTC);                                                                                   \
			fprintf(stdout, "[" name "] %lu.%06lu %s:%d: " msg, ts.tv_sec, ts.tv_nsec / 1000, __FILE__, __LINE__, ##args); \
			fflush(stdout);                                                                                                \
		}                                                                                                                      \
	while (0)
#define YANET_LOG(level, msg, args...) YANET_LOG_(#level, level, msg, ##args)

#define YANET_LOG_DEBUG(msg, args...) YANET_LOG(DEBUG, msg, ##args)
#define YANET_LOG_INFO(msg, args...) YANET_LOG(INFO, msg, ##args)
#define YANET_LOG_NOTICE(msg, args...) YANET_LOG(NOTICE, msg, ##args)
#define YANET_LOG_WARNING(msg, args...) YANET_LOG(WARNING, msg, ##args)
#define YANET_LOG_ERROR(msg, args...) YANET_LOG_("ERROR", ERR, msg, ##args)
