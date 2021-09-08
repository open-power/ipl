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
};

typedef std::map<ERR_TYPE, const char *> errMsgMapType;
const errMsgMapType errMsgMap = {{NONE, ""}};

// libphal specific errors base exception class
struct LibphalError : public std::exception {
	virtual ERR_TYPE errType() const noexcept = 0;
};

// SBE related functions errors base exception class

class SbeError final : public LibphalError {
public:
  SbeError(ERR_TYPE type, int fd, const char *fileName)
      : type(type), fd(fd), fileName(fileName) {}
  SbeError(ERR_TYPE type) : type(type), fd(-1), fileName(NULL){};

  virtual ~SbeError(){};

  ERR_TYPE errType() const noexcept override { return type; }

  const char *what() const noexcept override {
    auto msg = "UNSUPPORTED_ERROR_TYPE";
    auto errMsg = errMsgMap.find(type);
    if (errMsg != errMsgMap.end()) {
      msg = errMsg->second;
    }
    return msg;
  }

  int getFd() const noexcept { return fd; }

  const char *getFileName() const noexcept { return fileName; }

private:
  const ERR_TYPE type;
  int fd;
  const char *fileName;
};

} // namespace exception

using libphalerror_t = exception::LibphalError;
using sbeerror_t = exception::SbeError;

} // namespace libphal
