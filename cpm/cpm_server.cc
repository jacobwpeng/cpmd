/*
 * ==============================================================================
 *
 *       Filename:  cpm_server.cc
 *        Created:  04/11/15 21:39:40
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "cpm_server.h"

#include <alpha/compiler.h>
#include <alpha/logger.h>
#include <alpha/process_bus.h>
#include <alpha/net_address.h>
#include <alpha/event_loop.h>
#include <alpha/tcp_client.h>
#include <alpha/tcp_server.h>
#include <alpha/udp_server.h>
#include "cpm_message.h"
#include "cpm_protocol.h"
#include "cpm_client_info.h"

namespace cpm {
    const char* Server::kDefaultMessageServerIp = "0.0.0.0";
    Server::Server(alpha::EventLoop* loop, alpha::Slice bus_location)
        :register_server_port_(kDefaultRegisterServerPort)
         ,message_server_port_(kDefaultMessageServerPort)
         ,self_address_(0) 
         ,message_server_ip_(kDefaultMessageServerIp)
         ,loop_(loop), bus_location_(bus_location.ToString()) {
    }

    Server::~Server() = default;

    Server& Server::SetMessageServerIp(alpha::Slice ip) {
        message_server_ip_ = ip.ToString();
        return *this;
    }

    Server& Server::SetMessageServerPort(int port) {
        message_server_port_ = port;
        return *this;
    }

    Server& Server::SetRegisterServerPort(int port) {
        register_server_port_ = port;
        return *this;
    }

    bool Server::Run() {
        (void)loop_;
        using namespace std::placeholders;
        register_server_.reset (new alpha::UdpServer(loop_, "127.0.0.1", register_server_port_));
        register_server_->set_read_callback(std::bind(&Server::HandleInitCommand, this, 
                    _1, _2, _3));
        loop_->set_period_functor(std::bind(&Server::HandleBusMessage, this, _1));
        register_server_->Start();
        return true;
    }

    int Server::HandleInitCommand(const char* data, int len, std::string* out) {
        if (unlikely(len != static_cast<int>(sizeof(ProtocolMessage)))) {
            //TODO: HexDump(data)
            LOG_WARNING << "Truncated ProtocolMessage, len = " << len;
            return -1;
        }

        //TODO: Check MagicNum
        ProtocolMessage received;
        ::memcpy(&received, data, len);
        auto * req = received.as<HandShakeRequest>();
        //TODO: Check buffer_size
        ProtocolMessage m;
        auto * reply = m.as<HandShakeResponse>();
        LOG_INFO << "HandShakeRequest from " << req->name;

        auto input_bus_path = GetInputBusPath(req->name);
        auto input_bus = alpha::ProcessBus::RestoreOrCreate(
                input_bus_path, req->buffer_size, true);
        auto output_bus_path = GetOutputBusPath(req->name);
        auto output_bus = alpha::ProcessBus::RestoreOrCreate(
                output_bus_path, req->buffer_size, true);
        if (input_bus == nullptr || output_bus == nullptr) {
            LOG_WARNING << "Failed to create bus, req->name = " << req->name
                << ", req->buffer_size = " << req->buffer_size;
            reply->code = HandShakeError::kFailedToCreateBus;
        } else {
            auto addr = Address::Create(req->name);
            ClientPtr client(new ClientInfo(req->name, addr));
            client->SetInputBus(std::move(input_bus));
            client->SetOutputBus(std::move(output_bus));
            clients_.emplace(addr.ClientAddress(), std::move(client));
            reply->code = HandShakeError::kOk;
            assert (input_bus_path.size() < sizeof(reply->input_tunnel_path));
            assert (output_bus_path.size() < sizeof(reply->output_tunnel_path));
            //对方的output就是我们的input
            ::strncpy(reply->output_tunnel_path, input_bus_path.data(), input_bus_path.size());
            ::strncpy(reply->input_tunnel_path, output_bus_path.data(), output_bus_path.size());
        }
        out->assign(reinterpret_cast<const char*>(&m), sizeof(m));
        return 0;
    }

    int Server::HandleBusMessage(int64_t iteration) {
        bool busy = false;
        for (auto& p : clients_) {
            Message * m = nullptr;
            while (p.second->ReadMessage(&m)) {
                busy = true;
                m->SetSourceAddress(p.second->addr());
                auto remote_addr = m->RemoteAddress();
                if (remote_addr.NodeAddress() == self_address_) {
                    HandleLocalMessage(m);
                } else {
                    HandleRemoteMessage(m);
                }
            }
        }
        return busy ? alpha::EventLoop::kBusy : alpha::EventLoop::kIdle;
    }

    void Server::HandleLocalMessage(Message* m) {
        assert (m->RemoteAddress().NodeAddress() == self_address_);
        auto client_addr = m->RemoteAddress().ClientAddress(); 
        auto it = clients_.find(client_addr);
        auto source_name = clients_.find(m->SourceAddress().ClientAddress())->second->name();
        if (it == clients_.end()) {
            LOG_INFO << "Drop local message from " << source_name
                <<", to address " << client_addr;
        } else {
            bool ok = it->second->WriteMessage(m);
            LOG_INFO_IF(!ok) << "Drop local message due to output bus full from " 
                << source_name <<", to address " << it->second->name();
            DLOG_INFO << "Forward 1 message from " << source_name
                << ", to " << it->second->name();
        }
    }

    void Server::HandleRemoteMessage(Message* m) {
    }

    std::string Server::GetInputBusPath(alpha::Slice name) const {
        return GetBusPath("%s/%s_input.bus", name);
    }

    std::string Server::GetOutputBusPath(alpha::Slice name) const {
        return GetBusPath("%s/%s_output.bus", name);
    }

    std::string Server::GetBusPath(alpha::Slice fmt, alpha::Slice name) const {
        char buf[kMaxPathSize];
        LOG_WARNING_IF(bus_location_.size() + name.size() >= static_cast<size_t>(kMaxPathSize)) <<
            "length of path exceed kMaxPathSize, bus_location_ = " << bus_location_
            << ", name = " << name.data();
        snprintf(buf, sizeof(buf), fmt.data(), bus_location_.data(), name.data());
        DLOG_INFO << "name = " << name.data()
            << ", path = " << buf;
        return buf;
    }
}
