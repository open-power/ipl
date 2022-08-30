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
	PDBG_TARGET_NOT_FOUND,
	PDBG_TARGET_PARENT_NOT_FOUND,
	PDBG_TARGET_INVALID,
	SBE_STATE_READ_FAIL,
	SBE_STATE_WRITE_FAIL,
	SBE_CHIPOP_NOT_ALLOWED,
	SBE_INVALID_STATE,
	SBE_FFDC_GET_FAILED,
	SBE_CMD_FAILED,
	SBE_CMD_TIMEOUT,
	SBE_FFDC_NO_DATA,
	PDBG_INIT_FAIL,
	HWP_EXECUTION_FAILED,
	FILE_OPERATION_FAILED,
	DEVTREE_ATTR_READ_FAIL,
	DEVTREE_ATTR_WRITE_FAIL,
	DEVTREE_PRI_PROC_NOT_FOUND,
	PDBG_FSI_READ_FAIL,
	PDBG_FSI_WRITE_FAIL,
};

// Error type to message map.
typedef std::map<ERR_TYPE, const char *> errMsgMapType;
const errMsgMapType errMsgMap = {
    {NONE, ""},
    {PDBG_TARGET_NOT_OPERATIONAL, "PDBG target not in operational state"},
    {PDBG_TARGET_NOT_FOUND, "PDBG target not found"},
    {PDBG_TARGET_PARENT_NOT_FOUND, "PDBG parent target not found"},
    {PDBG_TARGET_INVALID, "PDBG target is invalid"},
    {SBE_STATE_READ_FAIL, "FAILED to read SBE state"},
    {SBE_STATE_READ_FAIL, "FAILED to write SBE state"},
    {SBE_CHIPOP_NOT_ALLOWED, "SBE chip-op not allowed"},
    {SBE_FFDC_GET_FAILED, "SBE FFDC get functionn failed"},
    {SBE_CMD_FAILED, "SBE command reported error"},
    {SBE_CMD_TIMEOUT, "SBE command timeout"},
    {SBE_FFDC_NO_DATA, "SBE FFDC No failure data"},
    {PDBG_INIT_FAIL, "PDBG initialisation failed"},
    {HWP_EXECUTION_FAILED, "HWP execution failed"},
    {FILE_OPERATION_FAILED, "File operation failed"},
    {DEVTREE_ATTR_READ_FAIL, "Device Tree attribute read fail"},
    {DEVTREE_ATTR_WRITE_FAIL, "Device Tree attribute write fail"},
    {DEVTREE_PRI_PROC_NOT_FOUND, "Device Tree primary processor not found"},
    {PDBG_FSI_READ_FAIL, "FSI read failed"},
    {PDBG_FSI_WRITE_FAIL, "FSI write failed"}};

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

// Exception handling for dump specific functions
class DumpError final : public phalError
{
       public:
	DumpError(ERR_TYPE type) : type(type)
	{
	}

	DumpError &operator=(DumpError &&) = default;
	DumpError(DumpError &&) = default;

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

       private:
	/* Error type */
	ERR_TYPE type;
};

// PDBG related functions errors base exception class
// Exception class includes all the informations required
// advanced error handling for api users.
// Library users should watch this exception during PDBG
// namespace specific api calls to get details of the
// function errors
class PdbgError final : public phalError
{
       public:
	PdbgError(ERR_TYPE type) : type(type){};
	PdbgError() : type(NONE){};

	PdbgError &operator=(PdbgError &&) = default;
	PdbgError(PdbgError &&) = default;

	virtual ~PdbgError(){};

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

       private:
	/* Error type */
	ERR_TYPE type;
};
} // namespace exception

using phalError_t = openpower::phal::exception::phalError;
using sbeError_t = openpower::phal::exception::SbeError;
using dumpError_t = openpower::phal::exception::DumpError;
using pdbgError_t = openpower::phal::exception::PdbgError;

} // namespace openpower::phal
