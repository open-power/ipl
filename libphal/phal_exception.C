#include "log.H"
#include "phal_exception.H"

namespace openpower::phal
{
namespace exception
{

using namespace openpower::phal::logging;

SbeError::~SbeError()
{
	for(const auto& iter : ffdcFileList)
	{
		if(fs::remove(iter.second.second) == false)
		{
			// Log error message and exit from destructor
			log(level::ERROR, "File(%s) remove failed(errnno:%d)",
			    iter.second.second.c_str(), errno);
		}
	}
}
} // namespace exception
} // namespace openpower::phal
