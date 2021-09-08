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
};

// Error type to message map.
typedef std::map<ERR_TYPE, const char *> errMsgMapType;
const errMsgMapType errMsgMap = {{NONE, ""}};

// phal specific errors base exception class
struct phalError : public std::exception {
	virtual ERR_TYPE errType() const noexcept = 0;
};

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

	virtual ~SbeError(){};

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
