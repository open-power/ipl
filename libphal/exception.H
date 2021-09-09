#pragma once

#include <exception>
#include <map>
#include <string>

namespace libphal
{
namespace exception
{

enum ERR_TYPE {
	NONE = 0,
	SBE_STATE_READ_FAIL,
	SBE_CHIPOP_NOT_ALLOWED,
	SBE_FFDC_GET_FAILED,
	SBE_CHIPOP_FAILED,
	SBE_CHIPOP_TIMEOUT,
};

typedef std::map<ERR_TYPE, const char *> errMsgMapType;
const errMsgMapType errMsgMap = {
    {NONE, ""},
    {SBE_STATE_READ_FAIL, "FAILED to read SBE state"},
    {SBE_CHIPOP_NOT_ALLOWED, "SBE chip-op not allowed"},
    {SBE_FFDC_GET_FAILED, "SBE FFDC get functionn failed"},
    {SBE_CHIPOP_FAILED, "SBE chip-op reported error"},
    {SBE_CHIPOP_TIMEOUT, "SBE chip-op timeout"}};

// libphal specific errors base exception class
struct LibphalError : public std::exception {
	virtual ERR_TYPE errType() const noexcept = 0;
};

// libphal specific errors base exception class

class SbeError final : public LibphalError
{
       public:
	SbeError(ERR_TYPE type, int fd, const char *fileName) :
	    type(type), fd(fd), fileName(fileName)
	{
	}
	SbeError(ERR_TYPE type) : type(type), fd(-1), fileName(NULL){};

	virtual ~SbeError(){};

	ERR_TYPE errType() const noexcept override
	{
		return type;
	}

	const char *what() const noexcept override
	{
		auto msg = "UNSUPPORTED_ERROR_TYPE";
		auto errMsg = errMsgMap.find(type);
		if (errMsg != errMsgMap.end()) {
			msg = errMsg->second;
		}
		return msg;
	}

	int getFd() const noexcept
	{
		return fd;
	}

	const char *getFileName() const noexcept
	{
		return fileName;
	}

       private:
	const ERR_TYPE type;
	int fd;
	const char *fileName;
};

} // namespace exception

using libphalError_t = exception::LibphalError;
using sbeError_t = exception::SbeError;

} // namespace libphal
