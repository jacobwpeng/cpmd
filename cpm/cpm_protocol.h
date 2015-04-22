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

#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>
#include "cpm_address.h"

namespace cpm {
    static const int kMaxNameSize = 1 << 7;
    static const int kMaxBufferSize = 1 << 20;
    static const int kMaxPathSize = 1 << 7;
    static const int kMaxNodePathSize = 1 << 7;
    static const int kMaxProtocolDataSize = 1 << 10;
    static const int kMaxIpSize = 16; //255.255.255.255\0

    enum class MessageType : int16_t {
        kClientHandShake = 1,
        kResolveRequest = 2,
        kResolveResponse = 3,
    };

    struct ProtocolMessage {
        MessageType type;
        uint32_t magic;
        uint16_t len;
        char data[kMaxProtocolDataSize];

        template<typename T>
        typename std::enable_if< 
                 std::is_pointer<T>::value 
                 && std::is_pod<typename std::remove_pointer<T>::type>::value 
                 && sizeof(typename std::remove_pointer<T>::type) <= kMaxProtocolDataSize
            , T>::type as() {
                return reinterpret_cast<T>(data);
        }

        template<typename T>
        typename std::enable_if< 
                 std::is_pointer<T>::value 
                 && std::is_pod<typename std::remove_pointer<T>::type>::value 
                 && std::is_const<typename std::remove_pointer<T>::type>::value
                 && sizeof(typename std::remove_pointer<T>::type) <= kMaxProtocolDataSize
            , T>::type as() const{
                return reinterpret_cast<T>(data);
        }

        int size() const;
        alpha::Slice Serialize() const;
    };
    static const int kMinProtocolMessageSize = offsetof(ProtocolMessage, data);

    static const uint32_t kProtocolMessageMagic = 0x12345678;

    struct HandShakeRequest {
        uint32_t buffer_size;
        char name[kMaxNameSize];
    };

    enum class HandShakeError : int16_t {
        kOk = 0,
        kAlreadyRegistered = -100,
        kFailedToCreateBus = -101,
    };

    struct HandShakeResponse {
        HandShakeError code;
        char input_tunnel_path[kMaxPathSize];
        char output_tunnel_path[kMaxPathSize];
    };

    enum class ResolveServerError : int16_t {
        kOk = 0,
        kInvalidNode = -200,
        kNodeNotFound = -201,
    };

    enum class ResolveRequestType : int16_t {
        kInitializeSelf = 1,
        kLookupNode = 2,
    };

    enum class ResolveResponseType : int16_t {
        kUpdateSelf = 1,
        kUpdateCache = 2,
    };

    struct ResolveRequest {
        ResolveRequestType type;
        Address::NodeAddressType self_addr;
        int32_t cpmd_port;
        Address::NodeAddressType peer_addr;

        class Builder {
            public:
                Builder(ProtocolMessage* m);
                Builder& SetType(ResolveRequestType type);
                Builder& SetSelfAddress(Address::NodeAddressType addr);
                Builder& SetPort(int32_t port);
                Builder& SetPeerAddress(Address::NodeAddressType peer_addr);

            private:
                ProtocolMessage* m_;
        };
    };

    struct ResolveResponse {
        ResolveServerError code;
        ResolveResponseType type;
        Address::NodeAddressType node_addr;
        int32_t node_port;
        char node_ip[kMaxIpSize];
        char node_path[kMaxNodePathSize];

        class Builder {
            public:
                Builder(ProtocolMessage* m);
                Builder& SetCode(ResolveServerError code);
                Builder& SetType(ResolveResponseType type);
                Builder& SetNodeAddress(Address::NodeAddressType addr);
                Builder& SetNodePort(int32_t port);
                Builder& SetNodeIp(alpha::Slice ip);
                Builder& SetNodePath(alpha::Slice path);

            private:
                ProtocolMessage* m_;
        };

    };

#define ASSERT_POD_TYPE(type) \
    static_assert (std::is_pod<type>::value, #type" must be POD type")
#define ASSERT_TYPE_SIZE(type) \
    static_assert (sizeof(type) <= kMaxProtocolDataSize, \
            "sizeof("#type") cannot be larger than kMaxProtocolDataSize");

    static_assert(kMaxProtocolDataSize <= 
            std::numeric_limits<decltype(ProtocolMessage::len)>::max(), 
            "max value of ProtocolMessage::len must be larger thant kMaxProtocolDataSize");
    ASSERT_TYPE_SIZE(HandShakeRequest);
    ASSERT_TYPE_SIZE(HandShakeResponse);
    ASSERT_TYPE_SIZE(ResolveRequest);
    ASSERT_TYPE_SIZE(ResolveResponse);
    ASSERT_POD_TYPE(ProtocolMessage);
    ASSERT_POD_TYPE(HandShakeRequest);
    ASSERT_POD_TYPE(HandShakeResponse);
    ASSERT_POD_TYPE(ResolveRequest);
    ASSERT_POD_TYPE(ResolveResponse);

#undef ASSERT_POD_TYPE
#undef ASSERT_TYPE_SIZE
}

#endif   /* ----- #ifndef __CPM_PROTOCOL_H__  ----- */

