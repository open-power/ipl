#pragma once

#include <exception>

namespace openpower::phal
{
namespace exception
{

enum ERR_TYPE {
	NONE = 0,
};

typedef std::map<ERR_TYPE, const char *> errMsgMapType;
const errMsgMapType errMsgMap = {{NONE, ""}};

// phal specific errors base exception class
struct phalError : public std::exception {
	virtual ERR_TYPE errType() const noexcept = 0;
};

} // namespace exception

using phalError_t = openpower::phal::exception::phalError;

} // namespace openpower::phal
