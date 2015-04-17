/*
 * =============================================================================
 *
 *       Filename:  cpm_protocol_message_codec.cc
 *        Created:  04/15/15 16:06:54
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "cpm_protocol_message_codec.h"

#include <alpha/logger.h>

#include "cpm_protocol.h"

namespace cpm {
    void ProtocolMessageCodec::OnRead(alpha::TcpConnectionPtr conn, 
            alpha::TcpConnectionBuffer* buf) {
        auto data = buf->Read();
        if (data.size() < kMinProtocolMessageSize) return;
        const ProtocolMessage * m = reinterpret_cast<const ProtocolMessage*>(data.data());
        if (m->magic != kProtocolMessageMagic) {
            LOG_WARNING << "Invalid ProtocolMessage from " << conn->PeerAddr()
                << ", m->magic = " << m->magic;
            buf->ConsumeBytes(data.size());
            return;
        }
        if (m->len >= kMaxProtocolDataSize) {
            LOG_WARNING << "Invalid ProtocolMessage from " << conn->PeerAddr()
                << ", m->len = " << m->len;
            buf->ConsumeBytes(data.size());
            return;
        }
        DLOG_INFO << "New ProtocolMessage from " << conn->PeerAddr()
            << ", m->type = " << 
            static_cast<typename std::underlying_type<decltype(m->type)>::type>(m->type) 
            << ", m->len = " << m->len;
        if (message_callback_) {
            message_callback_(conn, *m);
        }
        buf->ConsumeBytes(data.size());
    }
}
