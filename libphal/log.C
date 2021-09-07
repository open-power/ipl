#include "log.H"

#include <iostream>

namespace libphal
{

void phalLog(int logLevel, const char *fmt, ...)
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

} // namespace libphal
