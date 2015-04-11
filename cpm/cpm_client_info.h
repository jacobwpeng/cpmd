/*
 * ==============================================================================
 *
 *       Filename:  cpm_client_info.h
 *        Created:  04/11/15 21:20:07
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include <string>
#include <memory>
#include <alpha/slice.h>
#include "cpm_address.h"

namespace alpha {
    class ProcessBus;
}

namespace cpm {
    class Message;
    class ClientInfo {
        private:
            using ProcessBusPtr = std::unique_ptr<alpha::ProcessBus>;

        public:
            ClientInfo(alpha::Slice name, const Address& addr);
            ~ClientInfo();

            void SetInputBus(ProcessBusPtr&& bus);
            void SetOutputBus(ProcessBusPtr&& bus);

            bool WriteMessage(const Message* m);
            bool ReadMessage(Message** m);

            Address addr() const;
            std::string name() const;

        private:
            std::string name_;
            Address addr_;
            ProcessBusPtr input_;
            ProcessBusPtr output_;
    };
}
