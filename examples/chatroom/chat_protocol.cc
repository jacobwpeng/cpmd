/*
 * =============================================================================
 *
 *       Filename:  chat_protocol.cc
 *        Created:  04/23/15 16:13:34
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "chat_protocol.h"
#include <cstring>

namespace chat {
    Message::Builder::Builder() {
        auto m = new Message;
        ::memset(m, 0x0, sizeof(Message));
        m_.reset(m);
    }

    Message::Builder::~Builder() = default;

    Message::Builder& Message::Builder::SetType(MessageType type) {
        m_->type = type;
        return *this;
    }

    Message::Builder& Message::Builder::SetName(const std::string& name) {
        ::strncpy(m_->name, name.data(), sizeof(m_->name) - 1);
        m_->name[sizeof(m_->name) - 1] = '\0';
        return *this;
    }

    Message::Builder& Message::Builder::SetToName(const std::string& name) {
        ::strncpy(m_->to_name, name.data(), sizeof(m_->to_name) - 1);
        m_->to_name[sizeof(m_->to_name) - 1] = '\0';
        return *this;
    }

    Message::Builder& Message::Builder::SetWords(const std::string& words) {
        ::strncpy(m_->words, words.data(), sizeof(m_->words) - 1);
        m_->words[sizeof(m_->words) - 1] = '\0';
        return *this;
    }

    Message Message::Builder::Finish() {
        return *m_;
    }
}
