/*
 * =============================================================================
 *
 *       Filename:  cpm_message_cache_queue.h
 *        Created:  04/21/15 11:05:27
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __CPM_MESSAGE_CACHE_QUEUE_H__
#define  __CPM_MESSAGE_CACHE_QUEUE_H__

#include <queue>
#include "cpm_message.h"

namespace cpm {
    class MessageCacheQueue {
        public:
            MessageCacheQueue(int max_size);
            bool Empty() const;
            int Size() const;

            const Message* Front() const;
            Message PopFront();

            void Push(const Message& m, Message* droped);

        private:
            const int max_size_;
            std::queue<Message> messages_;
    };
}

#endif   /* ----- #ifndef __CPM_MESSAGE_CACHE_QUEUE_H__  ----- */
