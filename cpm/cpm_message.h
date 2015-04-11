/*
 * ==============================================================================
 *
 *       Filename:  cpm_message.h
 *        Created:  04/11/15 11:30:42
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __CPM_MESSAGE_H__
#define  __CPM_MESSAGE_H__

#include <alpha/slice.h>
#include "cpm_address.h"

namespace cpm {
    class Message {
        public:
            static const int kMaxDataSize = (1 << 16) - 100;
            static const int kMinSize;
            static const int kMaxSize;
            static Message Default();

            alpha::Slice Data() const;
            bool SetData(alpha::Slice data);

            void SetSourceAddress(const Address& from);
            void SetRemoteAddress(const Address& to);
            Address RemoteAddress() const;
            Address SourceAddress() const;
            void Clear();
            int Size() const;

        private:
            Message() = default;
            Address from_;
            Address to_;
            int len_;
            char data_[kMaxDataSize];
    };
}

#endif   /* ----- #ifndef __CPM_MESSAGE_H__  ----- */
