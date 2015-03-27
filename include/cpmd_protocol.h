/*
 * =============================================================================
 *
 *       Filename:  cpmd_protocol.h
 *        Created:  03/26/15 11:35:49
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __CPMD_PROTOCOL_H__
#define  __CPMD_PROTOCOL_H__

#include <memory>

namespace cpmd {
    const static std::size_t kMaxNameSize = 1 << 7;
    const static std::size_t kMaxBufferSize = 1 << 20;
    const static std::size_t kMaxPathSize = 1 << 7;
    struct HandShakeRequest {
        uint32_t buffer_size;
        char name[kMaxNameSize];
    };

    enum class HandShakeError {
        kOk = 0,
        kAlreadyRegistered = -100,
    };

    struct HandShakeResponse {
        HandShakeError code;
        char input_tunnel_path[kMaxPathSize];
        char output_tunnel_path[kMaxPathSize];
    };

    struct NameLookupRequest {
        char name[kMaxNameSize];
    };

    struct NameLookupResponse {
        uint64_t addr;
    };
}

#endif   /* ----- #ifndef __CPMD_PROTOCOL_H__  ----- */
