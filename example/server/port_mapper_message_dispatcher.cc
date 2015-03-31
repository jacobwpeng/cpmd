/*
 * =============================================================================
 *
 *       Filename:  port_mapper_message_dispatcher.cc
 *        Created:  03/30/15 16:41:08
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "port_mapper_message_dispatcher.h"

#include <cassert>
#include <alpha/logger.h>
#include <alpha/compiler.h>
#include "cpmd.h"

namespace cpmd {
    MessageDispatcher::MessageDispatcher() {
    }

    MessageDispatcher::~MessageDispatcher() = default;

    int MessageDispatcher::Routine(uint64_t iteration) {
        bool idle = true;
        const int kMaxMessageSize = sizeof(Message);
        const int kMinMessageSize = offsetof(Message, data);
        for (const auto & p : bus_map_) {
            int len;
            char * data = p.second.output->Read(&len);
            if (data != nullptr) {
                idle = false;
                if (unlikely(len > kMaxMessageSize || len <= kMinMessageSize)) {
                    LOG_WARNING << "Invalid message from client output bus, len = " 
                        << len << ", path = " << p.second.output->filepath();
                } else {
                    Message* m = reinterpret_cast<Message*>(data);
                    auto addr = m->addr;
                    auto it = bus_map_.find(addr);
                    if (it == bus_map_.end()) {
                        LOG_INFO << "cache missing for addr = " << addr;
                        continue;
                    } else {
                        bool ok = it->second.input->Write(data, len);
                        if (unlikely(!ok)) {
                            LOG_WARNING << "Write to client input failed, addr = "
                                << it->first;
                        }
                        LOG_INFO << "Forward message from addr = " << p.first
                            << " to addr = " << it->first << ", len = " << len;
                    }
                }
            }
        }

        return idle ? 0 : 1;
    }

    bool MessageDispatcher::AddClient(uint32_t addr, 
            MessageDispatcher::ProcessBusPtr&& input, 
            MessageDispatcher::ProcessBusPtr&& output) {
        auto it = bus_map_.find(addr);
        if (it != bus_map_.end()) {
            return false;
        }

        ProcessBusTunnel tunnel;
        tunnel.input = std::move(input);
        tunnel.output = std::move(output);
        bus_map_.emplace(addr, std::move(tunnel));
        LOG_INFO << "Add client, addr = " << addr;
        return true;
    }
}
