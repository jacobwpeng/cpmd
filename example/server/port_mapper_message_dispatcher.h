/*
 * =============================================================================
 *
 *       Filename:  port_mapper_message_dispatcher.h
 *        Created:  03/30/15 16:16:26
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __PORT_MAPPER_MESSAGE_DISPATCHER_H__
#define  __PORT_MAPPER_MESSAGE_DISPATCHER_H__

#include <memory>
#include <map>
#include <string>
#include <alpha/process_bus.h>

namespace cpmd {
    class MessageDispatcher {
        private:
            using ProcessBusPtr = std::unique_ptr<alpha::ProcessBus>;

        public:
            MessageDispatcher();
            ~MessageDispatcher();

            int Routine(uint64_t iteration);

            bool AddClient(uint32_t addr, ProcessBusPtr&& input, ProcessBusPtr&& output);

        private:
            struct ProcessBusTunnel {
                ProcessBusPtr input;
                ProcessBusPtr output;
            };
            using ProcessBusMap = std::map<uint32_t, ProcessBusTunnel>;

            ProcessBusMap bus_map_;
    };
}

#endif   /* ----- #ifndef __PORT_MAPPER_MESSAGE_DISPATCHER_H__  ----- */
