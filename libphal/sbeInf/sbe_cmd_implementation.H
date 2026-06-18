#pragma once

#include <stdio.h>
#include <fapi2.H>
#include <vector>
#include <cstdint>
#include <cstring>
#include "sbe_cmd_protocol.H"
#include "plat/plat_sbe_fifo.H"

namespace sbeIntf {

inline std::vector<uint8_t> toByteVector(const std::vector<uint32_t>& words)
{
    std::vector<uint8_t> bytes(words.size() * sizeof(uint32_t));
    std::memcpy(bytes.data(), words.data(), bytes.size());
    return bytes;
}


inline ResponseInfo decodeResponse(const std::vector<uint8_t>& rawBytes)
{
    ResponseInfo out;
    const size_t wordSize = sizeof(uint32_t);
    const size_t wordCount = rawBytes.size() / wordSize;
    const uint32_t* words = reinterpret_cast<const uint32_t*>(rawBytes.data());

    if (wordCount < 3)
    {
        return out;
    }

    uint32_t trailerOffset = ntohl(words[wordCount - 1]);
    size_t headerPos = wordCount - trailerOffset;

    if (headerPos + 1 >= wordCount)
    {
        return out;
    }

    out.primaryStatus = ntohl(words[headerPos + 1]) >> 16;
    out.secondaryStatus = ntohl(words[headerPos + 1]) & 0xFFFF;

    size_t payloadSize = headerPos * wordSize;
    out.payload.assign(rawBytes.begin(), rawBytes.begin() + payloadSize);

    size_t ffdcStart = (headerPos + 2) * wordSize;
    if (ffdcStart + 8 <= rawBytes.size())
    {
        size_t offset = ffdcStart;
        while (offset + 8 <= rawBytes.size())
        {
            const uint8_t* p = &rawBytes[offset];
            uint16_t magic = ntohs(*(uint16_t*)p);

            if (magic != 0xFBAD && magic != 0xFFDC)
                break;

            ResponseInfo::FFDC entry;
            entry.slid = ntohs(*(uint16_t*)(p + 4));
            entry.severity = *(p + 6);
            entry.chipId = *(p + 7);

            uint16_t wordLen = ntohs(*(uint16_t*)(p + 2));
            size_t byteLen = wordLen * wordSize;

            if (offset + byteLen > rawBytes.size())
                break;

            entry.rawData.assign(p, p + byteLen);
            out.ffdcList.push_back(std::move(entry));
            offset += byteLen;
        }
    }

    return out;
}


template <fapi2::TargetType T>
fapi2::ReturnCode executeChipOp(const fapi2::Target<T>& target,
                                const std::vector<uint32_t>& cmd_words,
                                std::vector<uint8_t>& data,
                                std::vector<ResponseInfo::FFDC>& ffdc)
{
    std::vector<uint8_t> request = toByteVector(cmd_words);

    uint32_t cmd = ntohl(cmd_words[1]);
    uint32_t timeoutMs = cmd::getRecommendedTimeoutMs(cmd);
    uint32_t out_len = cmd::getRecommendedSize(cmd);

    platform::setChipOpTimeout(target, timeoutMs);

    std::vector<uint8_t> response;
    auto rc = platform::transport(target, request, out_len, response);
    if (rc != fapi2::FAPI2_RC_SUCCESS) 
    {
        return rc;
    }
    
    auto decoded = decodeResponse(response);
    data = std::move(decoded.payload);
    ffdc = std::move(decoded.ffdcList);
    fprintf(stderr, "SBE response status: primary=0x%08X secondary=0x%08X, payload=%zu bytes, FFDC=%zu\n",
            decoded.primaryStatus, decoded.secondaryStatus,
            decoded.payload.size(), decoded.ffdcList.size());

    return (decoded.primaryStatus != 0 || decoded.secondaryStatus != 0)
               ? fapi2::FAPI2_RC_PLAT_ERR_SEE_DATA
               : fapi2::FAPI2_RC_SUCCESS;
}

template <fapi2::TargetType T>
fapi2::ReturnCode captureFfdc(const fapi2::Target<T>& target,
                                         std::vector<uint8_t>& discard,
                                         std::vector<ResponseInfo::FFDC>& outFfdc)
{
    std::vector<uint8_t> response;
    auto encoded = cmd::encodeGetFfdc();
    uint32_t cmd = ntohl(encoded[1]);
    uint32_t out_len = cmd::getRecommendedSize(cmd);
    auto rc = platform::transport(target, toByteVector(encoded), out_len, response);
    if (rc != fapi2::FAPI2_RC_SUCCESS) {
        return rc;
    }

    auto decoded = decodeResponse(response);
    if (!decoded.ffdcList.empty()) {
        outFfdc.insert(outFfdc.end(),
                       std::make_move_iterator(decoded.ffdcList.begin()),
                       std::make_move_iterator(decoded.ffdcList.end()));
        return fapi2::FAPI2_RC_SUCCESS;
    }

    return fapi2::FAPI2_RC_PLAT_ERR_SEE_DATA;
}

template <fapi2::TargetType T>
void mergeExtraFfdc(const fapi2::Target<T>& target,
                    std::vector<ResponseInfo::FFDC>& ffdc)
{
    std::vector<ResponseInfo::FFDC> extraFfdc;
    std::vector<uint8_t> discard;
    auto ffdcRc = captureFfdc(target, discard, extraFfdc);
    if (ffdcRc != fapi2::FAPI2_RC_SUCCESS && !extraFfdc.empty()) {
        ffdc.insert(ffdc.end(),
                    std::make_move_iterator(extraFfdc.begin()),
                    std::make_move_iterator(extraFfdc.end()));
    }
}

template <fapi2::TargetType T>
fapi2::ReturnCode getDump(const fapi2::Target<T>& target,
                          uint8_t type, uint8_t clock, uint8_t fa,
                          std::vector<uint8_t>& data,
                          std::vector<ResponseInfo::FFDC>& ffdc)
{
    auto cmd_words = cmd::encodeGetDump(type, clock, fa);
    auto rc = executeChipOp(target, cmd_words, data, ffdc);
    mergeExtraFfdc(target, ffdc);
    return rc;
}

}

