#pragma once

namespace libphal
{
namespace utils
{

struct BufPtr {
       public:
	//@brief Distructor for the object, free the allocated memory.
	~BufPtr()
	{
		free(dataPtr);
	}
	//@brief Returns the pointer to the data
	uint8_t **getPtr()
	{
		return &dataPtr;
	}
	// @brief Returns the stored data
	uint8_t *getData()
	{
		return dataPtr;
	}

       private:
	// The pointer to the data
	uint8_t *dataPtr = NULL;
};

} // namespace utils

using bufPtr_t = utils::BufPtr;

} // namespace libphal
