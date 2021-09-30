#include "log.H"
#include "phal_exception.H"

namespace openpower::phal
{
namespace exception
{

using namespace openpower::phal::logging;

SbeError::~SbeError()
{
	if (!fileName.empty()) {
		if (remove(fileName.c_str())) {
			// Log error message and exit from destructor
			log(level::ERROR, "File(%s) remove failed(errnno:%d)",
			    fileName.c_str(), errno);
		}
	}
}
} // namespace exception
} // namespace openpower::phal
