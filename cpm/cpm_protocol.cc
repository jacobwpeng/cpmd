/*
 * =============================================================================
 *
 *       Filename:  cpm_protocol.cc
 *        Created:  04/15/15 19:02:20
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "cpm_protocol.h"

#include <cstring>

namespace cpm {
    ResolveResponse::Builder::Builder(ProtocolMessage* m)
        :m_(m) {
        m_->len = 0;
    }
#define MinSizeWithField(type, field) \
    offsetof(type, field) + sizeof(type::field)

    ResolveResponse::Builder& ResolveResponse::Builder::SetCode(
            cpm::ResolveServerError code) {
        resp()->code = code;
        return EnsureLength(MinSizeWithField(ResolveResponse, code));
    }

    ResolveResponse::Builder& ResolveResponse::Builder::SetType(
            ResolveResponseType type) {
        resp()->type = type;
        return EnsureLength(MinSizeWithField(ResolveResponse, type));
    }

    ResolveResponse::Builder& ResolveResponse::Builder::SetNodeAddress(
            Address::NodeAddressType addr) {
        resp()->node_addr = addr;
        return EnsureLength(MinSizeWithField(ResolveResponse, node_addr));
    }

    ResolveResponse::Builder& ResolveResponse::Builder::SetNodeIp(alpha::Slice ip) {
        assert (sizeof(ResolveResponse::node_ip) > ip.size());
        ::strncpy(resp()->node_ip, ip.data(), ip.size());
        return EnsureLength(MinSizeWithField(ResolveResponse, node_ip));
    }

    ResolveResponse::Builder& ResolveResponse::Builder::SetNodePort(int32_t port) {
        resp()->node_port = port;
        return EnsureLength(MinSizeWithField(ResolveResponse, node_port));
    }

    ResolveResponse::Builder& ResolveResponse::Builder::SetNodePath(alpha::Slice path) {
        static_assert(MinSizeWithField(ResolveResponse, node_path) 
                == sizeof(ResolveResponse), "node_path must be the last element of "
                "ResolveResponse");
        assert (sizeof(ResolveResponse::node_path) > path.size());
        ::strncpy(resp()->node_path, path.data(), path.size());
        m_->len = offsetof(ResolveResponse, node_path) + path.size();
        return *this;
    }

    ResolveResponse* ResolveResponse::Builder::resp() {
        return m_->as<ResolveResponse*>();
    }

    ResolveResponse::Builder& ResolveResponse::Builder::EnsureLength(int len) {
        if (m_->len < len) {
            m_->len = len;
        }
        assert (m_->len <= sizeof(ProtocolMessage::data));
        return *this;
    }
}
