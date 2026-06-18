#pragma once

#include <vector>
#include <cstdint>
#include <endian.h>

namespace sbeIntf {
namespace cmd {

#define SBE_CMD(class, id) ((class << 8) | id)

// Utility to convert uint32_t to big-endian
inline uint32_t be(uint32_t val) {
    return htobe32(val);
}

// ------------------ Chip-op Encoders ------------------

inline std::vector<uint32_t> encodeGetDump(uint8_t type, uint8_t clock, uint8_t faCollect)
{
    uint32_t flags = ((faCollect & 0x1) << 16) | ((clock & 0x3) << 8) | (type & 0xf);
    return { be(3), be(SBE_CMD(0xAA, 0x01)), be(flags) };
}

inline std::vector<uint32_t> encodeGetFfdc()
{
    return { be(2), be(SBE_CMD(0xA8, 0x01)) };
}

inline std::vector<uint32_t> encodeResetFifo()
{
    return { be(2), be(SBE_CMD(0x21, 0x01)) };
}

inline uint32_t getRecommendedTimeoutMs(uint32_t cmdWord)
{
    switch (cmdWord)
    {
        case SBE_CMD(0xAA, 0x01):
            return 60000;
        default:
            return 5000;
    }
}

inline uint32_t getRecommendedSize(uint32_t cmd)
{
    switch (cmd)
    {
        case SBE_CMD(0xAA, 0x01):
            return 80 * 1024 * 1024;

        case SBE_CMD(0xA8, 0x01);
            return 64 * 1024;

        default:
            return 4096;
    }
}

} // namespace cmd
} // namespace sbeIntf
