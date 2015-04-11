/*
 * ==============================================================================
 *
 *       Filename:  cpm_protocol.h
 *        Created:  04/11/15 20:11:21
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __CPM_PROTOCOL_H__
#define  __CPM_PROTOCOL_H__

#include <type_traits>

namespace cpm {
    static const int kMaxNameSize = 1 << 7;
    static const int kMaxBufferSize = 1 << 20;
    static const int kMaxPathSize = 1 << 7;
    static const int kMaxProtocolDataSize = 1 << 10;

    enum class MessageType {
        kHandShake = 1,
    };

    struct ProtocolMessage {
        MessageType type;
        char data[kMaxProtocolDataSize];

        template<typename T>
        typename std::enable_if< std::is_pod<T>::value 
                             && !std::is_pointer<T>::value && 
                             sizeof(T) < kMaxProtocolDataSize
            , T>::type* as() {
                return reinterpret_cast<T*>(data);
        }
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

    static_assert (sizeof(HandShakeRequest) <= kMaxProtocolDataSize, 
            "sizeof(HandShakeRequest) cannot be larger than kMaxProtocolDataSize");
    static_assert (std::is_pod<ProtocolMessage>::value, 
            "ProtocolMessage must be POD type");
    static_assert (std::is_pod<HandShakeRequest>::value, 
            "HandShakeRequest must be POD type");
    static_assert (std::is_pod<HandShakeResponse>::value, 
            "HandShakeResponse must be POD type");
}

#endif   /* ----- #ifndef __CPM_PROTOCOL_H__  ----- */

