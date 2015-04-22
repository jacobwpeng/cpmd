/*
 * =============================================================================
 *
 *       Filename:  cpm_message_codec.cc
 *        Created:  04/22/15 14:27:47
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "cpm_message_codec.h"

#include <alpha/logger.h>

#include "cpm_message.h"

namespace cpm {
    void MessageCodec::SetOnMessage(const MessageCallback& cb) {
        message_callback_ = cb;
    }

    void MessageCodec::OnRead(alpha::TcpConnectionPtr conn, 
            alpha::TcpConnectionBuffer* buf) {
        auto data = buf->Read();
        auto nbytes = static_cast<int>(data.size());

        const char* ptr = data.data();
        while (nbytes >= Message::kMinSize) {
            const Message * m = reinterpret_cast<const Message*>(ptr);
            if (!m->Check()) {
                LOG_WARNING << "Check message failed, magic = " << m->magic();
                conn->Close();
                return;
            }

            if (m->Size() >= Message::kMaxSize) {
                LOG_WARNING << "Invalid Message from " << conn->PeerAddr()
                    << ", m->Size() = " << m->Size();
                conn->Close();
                return;
            }

            if (m->Size() > static_cast<int>(nbytes)) {
                //没接收完
                break;
            }

            DLOG_INFO << "New Message from " << conn->PeerAddr()
                << ", m->Size() = " << m->Size();
            if (message_callback_) {
                message_callback_(conn, *m);
            }
            nbytes -= m->Size();
            ptr += m->Size();
        }
        buf->ConsumeBytes(data.size() - nbytes);
    }
}
