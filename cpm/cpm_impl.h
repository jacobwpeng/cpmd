/*
 * ==============================================================================
 *
 *       Filename:  cpm_impl.h
 *        Created:  04/11/15 19:59:38
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __CPM_IMPL_H__
#define  __CPM_IMPL_H__

#include "cpm.h"
#include <string>
#include <memory>
#include <alpha/process_bus.h>

namespace cpm {
    class ClientImpl : public Client {
        public:
            ClientImpl(alpha::Slice name, Options options);
            virtual ~ClientImpl();

            Status Init();
            virtual Status SendMessage(const Message* m);
            virtual Status ReceiveMessage(Message** m);

        private:
            static const int kDefaultTimeout = 1000;
            std::string name_;
            Options options_;
            using ProcessBusPtr = std::unique_ptr<alpha::ProcessBus>;
            ProcessBusPtr input_;
            ProcessBusPtr output_;
    };
}

#endif   /* ----- #ifndef __CPM_IMPL_H__  ----- */
