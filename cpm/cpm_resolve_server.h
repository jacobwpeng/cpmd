/*
 * =============================================================================
 *
 *       Filename:  cpm_resolve_server.h
 *        Created:  04/15/15 14:26:52
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __CPM_RESOLVE_SERVER_H__
#define  __CPM_RESOLVE_SERVER_H__

#include <map>
#include <memory>
#include <alpha/tcp_connection.h>
#include "cpm_address.h"

namespace alpha {
    class TcpServer;
    class EventLoop;
}

namespace cpm {
    struct ProtocolMessage;
    struct ResolveRequest;
    class ProtocolMessageCodec;
    class ResolveServerConf;
    class ResolveServer {
        public:
            ResolveServer(alpha::EventLoop* loop, const char* file);
            ~ResolveServer();
            bool Run();

        private:
            void OnNewConnection(alpha::TcpConnectionPtr conn);
            void OnClose(alpha::TcpConnectionPtr conn);
            void OnMessage(alpha::TcpConnectionPtr& conn, const ProtocolMessage& m);
            void KickOffNode(alpha::TcpConnectionWeakPtr conn);
            bool HandleNodeInit(alpha::TcpConnectionPtr& conn, const ResolveRequest*, 
                    ProtocolMessage* reply);
            bool HandleNodeLookup(alpha::TcpConnectionPtr& conn, const ResolveRequest*, 
                    ProtocolMessage* reply);
            alpha::EventLoop* loop_;
            std::unique_ptr<ProtocolMessageCodec> protocol_message_codec_;
            std::unique_ptr<ResolveServerConf> conf_;
            std::unique_ptr<alpha::TcpServer> service_server_;
            std::map<Address::NodeAddressType, alpha::TcpConnectionPtr> connections_;
    };
}

#endif   /* ----- #ifndef __CPM_RESOLVE_SERVER_H__  ----- */
