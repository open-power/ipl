#pragma once

#include <exception>

namespace libphal
{
namespace exception
{

enum ERR_TYPE {
	NONE = 0,
};

typedef std::map<ERR_TYPE, const char *> errMsgMapType;
const errMsgMapType errMsgMap = {{NONE, ""}};

// libphal specific errors base exception class
struct LibphalError : public std::exception {
	virtual ERR_TYPE errType() const noexcept = 0;
};

} // namespace exception

using libphalerror_t = exception::LibphalError;

} // namespace libphal
