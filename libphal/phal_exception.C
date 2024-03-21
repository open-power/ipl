#include "log.H"
#include "phal_exception.H"

namespace openpower::phal
{
namespace exception
{
//taken from error_info_defs.H, including the header file breaks logging and
//debug collector as they need to include ekb in include path
enum errlSeverity_t
{
	FAPI2_ERRL_SEV_UNDEFINED     = 0x00, /// Used internally by ffdc mechanism
	FAPI2_ERRL_SEV_RECOVERED     = 0x10, /// Not seen by customer
	FAPI2_ERRL_SEV_PREDICTIVE    = 0x20, /// Error recovered but customer will see
	FAPI2_ERRL_SEV_UNRECOVERABLE = 0x40  /// Unrecoverable, general
};

using namespace openpower::phal::logging;

SbeError::SbeError(ERR_TYPE type, int fd, const fs::path fileName) : type(type)
{
	ffdcFileList.insert(
		std::make_pair(DEFAULT_SLID,
			std::make_tuple(FAPI2_ERRL_SEV_UNRECOVERABLE, fd, fileName)));
}
SbeError::~SbeError()
{
	for(const auto& iter : ffdcFileList)
	{
		auto tuple = iter.second;
		if(fs::remove(std::get<2>(tuple)) == false)
		{
			// Log error message and exit from destructor
			log(level::ERROR, "File(%s) remove failed(errnno:%d)",
			    std::get<2>(tuple), errno);
		}
	}
}
} // namespace exception
} // namespace openpower::phal
