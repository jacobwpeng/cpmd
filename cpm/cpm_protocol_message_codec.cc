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
        auto nbytes = data.size();

        const char* ptr = data.data();
        while (nbytes >= kMinProtocolMessageSize) {
            const ProtocolMessage * m = reinterpret_cast<const ProtocolMessage*>(ptr);
            if (m->magic != kProtocolMessageMagic) {
                LOG_WARNING << "Invalid ProtocolMessage from " << conn->PeerAddr()
                    << ", m->magic = " << m->magic;
                conn->Close();
                return;
            }

            if (m->len >= kMaxProtocolDataSize) {
                LOG_WARNING << "Invalid ProtocolMessage from " << conn->PeerAddr()
                    << ", m->len = " << m->len;
                conn->Close();
                return;
            }

            if (m->size() > static_cast<int>(nbytes)) {
                //没接收完
                break;
            }

            DLOG_INFO << "New ProtocolMessage from " << conn->PeerAddr()
                << ", m->type = " << 
                static_cast<typename std::underlying_type<decltype(m->type)>::type>(m->type) 
                << ", m->len = " << m->len;
            if (message_callback_) {
                message_callback_(conn, *m);
            }
            nbytes -= m->size();
            ptr += m->size();
        }
        buf->ConsumeBytes(data.size() - nbytes);
    }
}
