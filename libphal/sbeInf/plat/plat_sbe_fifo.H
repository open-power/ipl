#pragma once

extern "C" {
#include <libsbefifo.h>
}

#include <stdio.h>

#include <fapi2.H>
#include <vector>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <cassert>

namespace sbeIntf {
namespace platform {

template <fapi2::TargetType T>
void setChipOpTimeout(const fapi2::Target<T>& target, uint32_t timeoutMs)
{
    struct pdbg_target *pib = openpower::phal::utils::pdbg::getPibTarget(target);

    sbe_set_chipop_timeout(pib, timeoutMs);
}

template <fapi2::TargetType T>
fapi2::ReturnCode transport(const fapi2::Target<T>& target,
                            const std::vector<uint8_t>& request, uint32_t out_len,
                            std::vector<uint8_t>& response)
{

    uint8_t* out = nullptr;

    struct pdbg_target *pib = openpower::phal::utils::pdbg::getPibTarget(target);

    int rc = sbe_operation(pib,
                           const_cast<uint8_t*>(request.data()),
                           request.size(),
                           &out,
                           &out_len);

    if (rc == ETIMEDOUT)
    {
        return fapi2::FAPI2_RC_PLAT_ERR_SEE_DATA;
    }
    else if (rc != 0)
    {
	fprintf(stderr, "transport failed in plat: rc=0x%08X\n", rc);
        return fapi2::FAPI2_RC_PLAT_ERR_SEE_DATA;
    }

    if (!out || out_len == 0)
    {
        return fapi2::FAPI2_RC_PLAT_ERR_SEE_DATA;
    }

    response.assign(out, out + out_len);
    free(out);
    return fapi2::FAPI2_RC_SUCCESS;
}

} // namespace platform
} // namespace sbeIntf

