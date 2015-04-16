/*
 * =============================================================================
 *
 *       Filename:  cpm_protocol_message_codec.h
 *        Created:  04/15/15 15:57:33
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __CPM_PROTOCOL_MESSAGE_CODEC_H__
#define  __CPM_PROTOCOL_MESSAGE_CODEC_H__

#include <functional>
#include <alpha/tcp_connection.h>

namespace cpm {
    struct ProtocolMessage;

    class ProtocolMessageCodec {
        public:
            using MessageCallback = std::function<
                void(alpha::TcpConnectionPtr& conn, const ProtocolMessage&)>;
            void SetOnMessage(const MessageCallback& cb) {
                message_callback_ = cb;
            }
            void OnRead(alpha::TcpConnectionPtr conn, alpha::TcpConnectionBuffer* buf);

        private:
            MessageCallback message_callback_;
    };
}

#endif   /* ----- #ifndef __CPM_PROTOCOL_MESSAGE_CODEC_H__  ----- */
