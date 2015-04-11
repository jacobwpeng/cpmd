/*
 * ==============================================================================
 *
 *       Filename:  cpm_client_info.cc
 *        Created:  04/11/15 22:04:04
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "cpm_client_info.h"
#include <cassert>
#include <alpha/process_bus.h>
#include <alpha/logger.h>
#include "cpm_message.h"

namespace cpm {
    ClientInfo::ClientInfo(alpha::Slice name, const Address& addr)
        :name_(name.ToString()), addr_(addr) {
    }

    ClientInfo::~ClientInfo() = default;

    void ClientInfo::SetInputBus(ProcessBusPtr&& bus) {
        assert (bus);
        input_ = std::move(bus);
    }

    void ClientInfo::SetOutputBus(ProcessBusPtr&& bus) {
        assert (bus);
        output_ = std::move(bus);
    }

    bool ClientInfo::WriteMessage(const Message* m) {
        assert (m);
        return output_->Write(reinterpret_cast<const char*>(m), m->Size());
    }

    bool ClientInfo::ReadMessage(Message** m) {
        assert (m);
        int len;
        char* data = input_->Read(&len);
        if (data == nullptr) {
            return false;
        } else if (len < Message::kMinSize) {
            LOG_WARNING << "Drop truncated message from " << name_
                << ", len = " << len;
            return false;
        } else {
            *m = reinterpret_cast<Message*>(data);
            return true;
        }
    }

    std::string ClientInfo::name() const {
        return name_;
    }

    Address ClientInfo::addr() const {
        return addr_;
    }
}
