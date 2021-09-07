#pragma once

#include <syslog.h>

#include <cstdarg>

/* sys/syslog.h:
 * LOG_EMERG       0       system is unusable
 * LOG_ALERT       1       action must be taken immediately
 * LOG_CRIT        2       critical conditions
 * LOG_ERR         3       error conditions
 * LOG_WARNING     4       warning conditions
 * LOG_NOTICE      5       normal but significant condition
 * LOG_INFO        6       informational
 * LOG_DEBUG       7       debug-level messages
 */

namespace openpower::phal
{
namespace logging
{

/** @enum level
 *  @brief Enum for logging priority level
 */
enum class level {
	EMERG = LOG_EMERG,
	ALERT = LOG_ALERT,
	CRIT = LOG_CRIT,
	ERROR = LOG_ERR,
	WARNING = LOG_WARNING,
	NOTICE = LOG_NOTICE,
	INFO = LOG_INFO,
	DEBUG = LOG_DEBUG
};

constexpr auto VERBOSE_LEVEL = level::INFO;

/**
 * @brief Log the traces
 * @param[in] int type of log i.e. DBG, ERR, INFO
 * @param[in] fmt trace string
 * @param[in] ... variable set of arguments can be passed like %d,%s etc
 **/
void log(level logLevel, const char *fmt, ...);

} // namespace logging

using level_t = openpower::phal::logging::level;

} // namespace openpower::phal
