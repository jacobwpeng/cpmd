/*
 * =============================================================================
 *
 *       Filename:  port_mapper_command_server.h
 *        Created:  03/30/15 14:14:43
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __PORT_MAPPER_COMMAND_SERVER_H__
#define  __PORT_MAPPER_COMMAND_SERVER_H__

#include <memory>
#include <alpha/slice.h>
#include "cpmd_protocol.h"

namespace alpha {
    class EventLoop;
    class UdpServer;
}

namespace cpmd {
    class MessageDispatcher;
    class CommandServer {
        public:
            CommandServer(alpha::EventLoop* loop, MessageDispatcher* m);
            ~CommandServer();

            int Init(const alpha::Slice& file);
            void Run();

        private:
            int OnMessage(const char* in, size_t len, std::string* out);
            void OnHandShake(const HandShakeRequest* req, HandShakeResponse* resp);
            void OnNameLookup(const NameLookupRequest* req, NameLookupResponse* resp);

        private:
            int port_;
            alpha::EventLoop* loop_;
            MessageDispatcher* dispatcher_;
            std::string data_path_;
            std::unique_ptr<alpha::UdpServer> server_;
    };
}

#endif   /* ----- #ifndef __PORT_MAPPER_COMMAND_SERVER_H__  ----- */
