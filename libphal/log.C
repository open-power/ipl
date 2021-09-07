#include "log.H"

#include <iostream>

namespace openpower::phal::logging
{

void log(level logLevel, const char *fmt, ...)
{
	if (VERBOSE_LEVEL < logLevel) {
		return;
	}

	va_list ap;
	va_start(ap, fmt);

	vfprintf(stdout, fmt, ap);
	std::cout << "\n";

	va_end(ap);
}

} // namespace openpower::phal::logging
