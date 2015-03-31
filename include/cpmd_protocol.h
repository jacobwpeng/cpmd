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
    const static std::size_t kMaxProtocolDataSize = 1 << 10;

    enum class MessageType {
        kHandShake = 1,
        kNameLookup = 2,
    };

    struct ProtocolMessage {
        MessageType type;
        char data[kMaxProtocolDataSize];
    };

    struct HandShakeRequest {
        uint32_t buffer_size;
        char name[kMaxNameSize];
    };

    enum class HandShakeError {
        kOk = 0,
        kAlreadyRegistered = -100,
        kFailedToCreateBus = -101,
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
        uint32_t addr;
    };

    static_assert (sizeof(HandShakeRequest) <= kMaxProtocolDataSize, 
            "sizeof(HandShakeRequest) cannot be larger than kMaxProtocolDataSize");
    static_assert (sizeof(NameLookupRequest) <= kMaxProtocolDataSize,
            "sizeof(NameLookupRequest) cannot be larger than kMaxProtocolDataSize");
}

#endif   /* ----- #ifndef __CPMD_PROTOCOL_H__  ----- */
