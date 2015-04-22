/*
 * ==============================================================================
 *
 *       Filename:  cpm_server.h
 *        Created:  04/11/15 21:13:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __CPM_SERVER_H__
#define  __CPM_SERVER_H__

#include <map>
#include <memory>
#include <string>
#include <alpha/slice.h>
#include <alpha/tcp_connection.h>
#include "cpm_address.h"
#include "cpm_message_cache_queue.h"

namespace alpha {
    class NetAddress;
    class EventLoop;
    class UdpServer;
    class TcpServer;
    class TcpClient;
}

namespace cpm {
    class Node;
    class Message;
    class ClientInfo;
    struct ProtocolMessage;
    struct ResolveResponse;
    class MessageCodec;
    class ProtocolMessageCodec;
    class Server {
        public:
            Server(alpha::EventLoop* loop, alpha::Slice bus_location);
            ~Server();

            Server& SetRegisterServerPort(int port);
            Server& SetMessageServerPort(int port);
            Server& SetMessageServerIp(alpha::Slice ip);

            bool Run();

        private:
            using NodePtr = std::unique_ptr<Node>;
            using ClientPtr = std::unique_ptr<ClientInfo>;
            static const int kMaxCacheMessageSize = 100;

            int HandleInitCommand(const char* data, int len, std::string* reply);
            bool HandleRemoteMessage(const Message* m);
            int HandleBusMessage(int64_t);
            void HandleToLocalMessage(Message* m);
            void HandleToRemoteMessage(Message* m);
            void HandleResolveServerMessage(alpha::TcpConnectionPtr& conn,
                    const ProtocolMessage& m);
            void HandleRemoteNodeMessage(alpha::TcpConnectionPtr& conn, const Message& m);
            void OnConnectedToRemote(alpha::TcpConnectionPtr conn);
            void OnConnectToRemoteError(const alpha::NetAddress& addr);
            void OnConnectToRemoteClose(alpha::TcpConnectionPtr conn);
            bool IsResolveServerAddress(const alpha::NetAddress& addr);

            void OnConnectedToResolveServer(alpha::TcpConnectionPtr& conn);
            void OnConnectedToResolveServerError(const alpha::NetAddress& addr);
            void OnResolveServerDisconnected(alpha::TcpConnectionPtr& conn);
            void OnConnectedToRemoteNode(alpha::TcpConnectionPtr& conn);
            void OnConnectToRemoteNodeError(const alpha::NetAddress& addr);
            void OnRemoteNodeDisconnected(alpha::TcpConnectionPtr& conn);
            void ReconnectToResolveServer(const alpha::NetAddress& addr);
            void ReqeustNodeNetAddress(Address::NodeAddressType node_address);
            bool UpdateSelfAddress(const ResolveResponse* resp);
            bool UpdateNodeAddressCache(const ResolveResponse* resp);
            void RegisterNode(Node* node);

            Node* AddNewNode(Address::NodeAddressType node_address);
            Node* FindNode(Address::NodeAddressType node_address);
            Node* FindNodeByNetAddress(const alpha::NetAddress& net_address);
            std::string GetInputBusPath(alpha::Slice name) const;
            std::string GetOutputBusPath(alpha::Slice name) const;
            std::string GetBusPath(alpha::Slice fmt, alpha::Slice name) const;

            int register_server_port_;
            int message_server_port_;
            Address::NodeAddressType self_address_;
            std::string message_server_ip_;
            alpha::EventLoop * loop_;
            std::string bus_location_;
            alpha::TcpConnectionPtr resolve_server_conn_;
            std::unique_ptr<alpha::UdpServer> register_server_;
            std::unique_ptr<alpha::TcpServer> message_server_;
            std::unique_ptr<alpha::TcpClient> client_;
            std::unique_ptr<MessageCodec> message_codec_;
            std::unique_ptr<ProtocolMessageCodec> protocol_message_codec_;
            std::unique_ptr<alpha::NetAddress> resolve_server_address_;
            std::map<Address::NodeAddressType, NodePtr> nodes_;
            std::map<alpha::NetAddress, Node*> nodes_index_;
            std::map<Address::ClientAddressType, ClientPtr> clients_;
    };
}

#endif   /* ----- #ifndef __CPM_SERVER_H__  ----- */
