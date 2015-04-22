/*
 * =============================================================================
 *
 *       Filename:  cpm_message_cache_queue.cc
 *        Created:  04/21/15 11:08:04
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "cpm_message_cache_queue.h"

#include <cassert>

namespace cpm {
    MessageCacheQueue::MessageCacheQueue(int max_size)
        :max_size_(max_size) {
    }

    bool MessageCacheQueue::Empty() const {
        return messages_.empty();
    }

    int MessageCacheQueue::Size() const {
        return messages_.size();
    }

    const Message* MessageCacheQueue::Front() const {
        if (Empty()) {
            return nullptr;
        }
        return &messages_.front();
    }

    Message MessageCacheQueue::PopFront() {
        assert (!Empty());
        Message m = messages_.front();
        messages_.pop();
        return m;
    }

    void MessageCacheQueue::Push(const Message& m, Message* droped) {
        if (Size() == max_size_ && droped != nullptr) {
            *droped = PopFront();
        }

        assert (Size() < max_size_);
        messages_.push(m);
    }
}
