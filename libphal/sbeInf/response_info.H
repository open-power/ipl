#pragma once

#include <vector>
#include <cstdint>

namespace sbeIntf {

struct ResponseInfo
{
    struct FFDC
    {
        uint8_t slid = 0;
        uint8_t severity = 0;
        uint32_t chipId = 0;
        std::vector<uint8_t> rawData;
    };

    std::vector<uint8_t> payload;
    std::vector<FFDC> ffdcList;
    uint16_t primaryStatus = 0;
    uint16_t secondaryStatus = 0;
};

}

