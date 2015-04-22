/*
 * =============================================================================
 *
 *       Filename:  cpm_message_codec.h
 *        Created:  04/22/15 14:12:56
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __CPM_MESSAGE_CODEC_H__
#define  __CPM_MESSAGE_CODEC_H__

#include <functional>
#include <alpha/tcp_connection.h>

namespace cpm {
    class Message;
    class MessageCodec {
        public:
            using MessageCallback = std::function<
                void(alpha::TcpConnectionPtr& conn, const Message&)>;
            void SetOnMessage(const MessageCallback& cb);
            void OnRead(alpha::TcpConnectionPtr conn, alpha::TcpConnectionBuffer* buf);

        private:
            MessageCallback message_callback_;
    };
}

#endif   /* ----- #ifndef __CPM_MESSAGE_CODEC_H__  ----- */
