#pragma once

#include <exception>
#include <map>
#include <string>

namespace openpower::phal
{
namespace exception
{

// Error Type definition
enum ERR_TYPE {
	NONE = 0,
	PDBG_TARGET_NOT_OPERATIONAL,
	SBE_STATE_READ_FAIL,
	SBE_CHIPOP_NOT_ALLOWED,
	SBE_FFDC_GET_FAILED,
	SBE_CMD_FAILED,
	SBE_CMD_TIMEOUT,
	SBE_FFDC_NO_DATA,
};

// Error type to message map.
typedef std::map<ERR_TYPE, const char *> errMsgMapType;
const errMsgMapType errMsgMap = {
    {NONE, ""},
    {PDBG_TARGET_NOT_OPERATIONAL, "PDBG target not in operational state"},
    {SBE_STATE_READ_FAIL, "FAILED to read SBE state"},
    {SBE_CHIPOP_NOT_ALLOWED, "SBE chip-op not allowed"},
    {SBE_FFDC_GET_FAILED, "SBE FFDC get functionn failed"},
    {SBE_CMD_FAILED, "SBE command reported error"},
    {SBE_CMD_TIMEOUT, "SBE command timeout"},
    {SBE_FFDC_NO_DATA, "SBE FFDC No failure data"}};

// phal specific errors base exception class
struct phalError : public std::exception {
	virtual ERR_TYPE errType() const noexcept = 0;
}; // namespace exception

// libphal SBE specific errors base exception class
// SBE related functions errors base exception class
// Exception class includes all the required for SBE RAS
// specific requirements and PEL.
// Library users can watch this exception during SBE specific wrapper
// calls and use the the object specific helper function get the required
// data for error handling purpose.
//
// Note: Application user should remove the file after usage.
class SbeError final : public phalError
{
       public:
	SbeError(ERR_TYPE type, int fd, const std::string fileName) :
	    type(type), fd(fd), fileName(fileName)
	{
	}
	SbeError(ERR_TYPE type) : type(type), fd(-1){};
	SbeError() : type(NONE), fd(-1){};

	SbeError &operator=(SbeError &&) = default;
	SbeError(SbeError &&) = default;

	virtual ~SbeError();

	/* return error type */
	ERR_TYPE errType() const noexcept override
	{
		return type;
	}

	/* return error string */
	const char *what() const noexcept override
	{
		auto msg = "UNSUPPORTED_ERROR_TYPE";
		auto errMsg = errMsgMap.find(type);
		if (errMsg != errMsgMap.end()) {
			msg = errMsg->second;
		}
		return msg;
	}

	/* return file descriptor includes the FFDC data */
	int getFd() const noexcept
	{
		return fd;
	}

	/* return filename information */
	const std::string getFileName() const noexcept
	{
		return fileName;
	}

       private:
	/* Error type */
	ERR_TYPE type;

	/* FFDC File descriptor */
	int fd;

	/* FFDC File name */
	std::string fileName;
};

} // namespace exception

using phalError_t = openpower::phal::exception::phalError;
using sbeError_t = openpower::phal::exception::SbeError;

} // namespace openpower::phal
