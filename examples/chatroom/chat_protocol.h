/*
 * =============================================================================
 *
 *       Filename:  chat_protocol.h
 *        Created:  04/23/15 15:57:19
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __CHAT_PROTOCOL_H__
#define  __CHAT_PROTOCOL_H__

#include <cstdint>
#include <type_traits>
#include <string>
#include <memory>

namespace chat {
    enum class MessageType : int16_t {
        kRegister = 1,
        kHeartBeat = 2,
        kMessage = 3,
        kPrivateMessage = 4,
    };

    static const int kMaxNameSize = 1 << 4;
    static const int kMaxWordsSize = 1 << 6;
    struct Message {
        MessageType type;
        char name[kMaxNameSize];
        char to_name[kMaxNameSize];
        char words[kMaxWordsSize];

        class Builder {
            public:
                Builder();
                ~Builder();
                Builder& SetType(MessageType type);
                Builder& SetName(const std::string& name);
                Builder& SetToName(const std::string& name);
                Builder& SetWords(const std::string& name);
                Message Finish();

            private:
                std::unique_ptr<Message> m_;
        };
    };
}

#endif   /* ----- #ifndef __CHAT_PROTOCOL_H__  ----- */
