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

namespace alpha {
    class NetAddress;
    class EventLoop;
    class UdpServer;
    class TcpServer;
    class TcpClient;
}

namespace cpm {
    class Message;
    class ClientInfo;
    class Server {
        public:
            Server(alpha::EventLoop* loop, alpha::Slice bus_location);
            ~Server();

            Server& SetRegisterServerPort(int port);
            Server& SetMessageServerPort(int port);
            Server& SetMessageServerIp(alpha::Slice ip);

            bool Run();

        private:
            using ClientPtr = std::unique_ptr<ClientInfo>;
            static const int kDefaultRegisterServerPort = 8123;
            static const int kDefaultMessageServerPort = 8123;
            static const char* kDefaultMessageServerIp;

            int HandleInitCommand(const char* data, int len, std::string* reply);
            int HandleBusMessage(int64_t);
            void HandleLocalMessage(Message* m);
            void HandleRemoteMessage(Message* m);
            std::string GetInputBusPath(alpha::Slice name) const;
            std::string GetOutputBusPath(alpha::Slice name) const;
            std::string GetBusPath(alpha::Slice fmt, alpha::Slice name) const;

            int register_server_port_;
            int message_server_port_;
            Address::NodeAddressType self_address_;
            std::string message_server_ip_;
            alpha::EventLoop * loop_;
            std::string bus_location_;
            std::unique_ptr<alpha::UdpServer> register_server_;
            std::unique_ptr<alpha::TcpServer> message_server_;
            std::unique_ptr<alpha::TcpClient> message_dispater_;
            std::map<Address::NodeAddressType, alpha::TcpConnectionPtr> nodes_;
            std::map<Address::NodeAddressType, alpha::TcpConnectionPtr> out_links_;
            std::map<Address::ClientAddressType, ClientPtr> clients_;
    };
}

#endif   /* ----- #ifndef __CPM_SERVER_H__  ----- */
