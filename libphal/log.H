#pragma once
#include <stdarg.h>

#define PHAL_EMERG 0
#define PHAL_ALERT 1
#define PHAL_CRIT 2
#define PHAL_ERROR 3
#define PHAL_WARNING 4
#define PHAL_NOTICE 5
#define PHAL_INFO 6
#define PHAL_DEBUG 7

constexpr auto VERBOSE_LEVEL = PHAL_INFO;

namespace libphal
{
/**
 * @brief Log the traces
 * @param[in] int type of log i.e. DBG, ERR, INFO
 * @param[in] fmt trace string
 * @param[in] ... variable set of arguments can be passed like %d,%s etc
 * @return NULL
 **/
void phalLog(int logLevel, const char *fmt, ...);

} // namespace libphal
