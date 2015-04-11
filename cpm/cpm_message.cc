/*
 * ==============================================================================
 *
 *       Filename:  cpm_message.cc
 *        Created:  04/11/15 14:31:04
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "cpm_message.h"
#include <cstring>
#include <type_traits>

static_assert (std::is_pod<cpm::Message>::value, "Message must be POD type");

namespace cpm {
    const int Message::kMinSize = offsetof(Message, data_);
    const int Message::kMaxSize = offsetof(Message, data_) + kMaxDataSize;
    Message Message::Default() {
        Message m;
        m.Clear();
        return m;
    }
    alpha::Slice Message::Data() const {
        return alpha::Slice(data_, len_);
    }

    bool Message::SetData(alpha::Slice data) {
        if (data.size() > kMaxDataSize) {
            return false;
        } else {
            len_ = data.size();
            ::memcpy(data_, data.data(), data.size());
            return true;
        }
    }

    void Message::SetSourceAddress(const Address& from) {
        from_ = from;
    }

    void Message::SetRemoteAddress(const Address& to) {
        to_ = to;
    }

    void Message::Clear() {
        from_.Clear();
        to_.Clear();
        len_ = 0;
#ifndef NDEBUG
        ::memset(data_, 0x0, sizeof(data_));
#endif
    }

    int Message::Size() const {
        return kMinSize + len_;
    }

    Address Message::RemoteAddress() const {
        return to_;
    }

    Address Message::SourceAddress() const {
        return from_;
    }
}
