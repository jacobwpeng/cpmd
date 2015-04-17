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
#include "cpm_protocol_message_codec.h"

namespace cpm {
    const char* Server::kDefaultMessageServerIp = "0.0.0.0";
    Server::Server(alpha::EventLoop* loop, alpha::Slice bus_location)
        :register_server_port_(kDefaultRegisterServerPort)
         ,message_server_port_(kDefaultMessageServerPort)
         ,self_address_(Address::kLocalNodeAddress) 
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
        using namespace std::placeholders;
        loop_->set_period_functor(std::bind(&Server::HandleBusMessage, this, _1));

        protocol_message_codec_.reset (new ProtocolMessageCodec());
        protocol_message_codec_->SetOnMessage(std::bind(
                    &Server::HandleResolveServerMessage, this, _1, _2));

        register_server_.reset (new alpha::UdpServer(loop_, "127.0.0.1", 
                    register_server_port_));
        register_server_->set_read_callback(std::bind(
                    &Server::HandleInitCommand, this, _1, _2, _3));
        register_server_->Start();

        resolve_server_address_.reset (new alpha::NetAddress("127.0.0.1", 9999));
        client_.reset (new alpha::TcpClient(loop_));
        client_->SetOnConnected(std::bind(
                    &Server::OnConnectedToRemote, this, _1));
        client_->SetOnConnectError(std::bind(
                    &Server::OnConnectToRemoteError, this, _1));
        client_->SetOnClose(std::bind(
                    &Server::OnConnectToRemoteClose, this, _1));
        client_->ConnectTo(*resolve_server_address_);
        return true;
    }

    void Server::OnConnectedToRemote(alpha::TcpConnectionPtr conn) {
        if (IsResolveServerAddress(conn->PeerAddr())) {
            OnConnectedToResolveServer(conn);
        } else {
            OnConnectedToRemoteNode(conn);
        }
    }

    void Server::OnConnectToRemoteError(const alpha::NetAddress& addr) {
        if (IsResolveServerAddress(addr)) {
            OnConnectedToResolveServerError(addr);
        } else {
            OnConnectToRemoteNodeError(addr);
        }
    }

    void Server::OnConnectToRemoteClose(alpha::TcpConnectionPtr conn) {
        if (IsResolveServerAddress(conn->PeerAddr())) {
            OnResolveServerDisconnected(conn);
        } else {
            OnRemoteNodeDisconnected(conn);
        }
    }

    void Server::OnConnectedToRemoteNode(alpha::TcpConnectionPtr& conn) {
        LOG_INFO << "Connected to remote node " << conn->PeerAddr();
        // 看看有没有缓存啥要发的
    }

    void Server::OnConnectToRemoteNodeError(const alpha::NetAddress& addr) {
        LOG_WARNING << "Connect to remote node " << addr << " failed";
        // 看看是不是要把缓存的内容给清空掉
    }

    void Server::OnRemoteNodeDisconnected(alpha::TcpConnectionPtr& conn) {

    }

    bool Server::IsResolveServerAddress(const alpha::NetAddress& addr) {
        return addr == *resolve_server_address_;
    }

    int Server::HandleInitCommand(const char* data, int len, std::string* out) {
        if (unlikely(len != static_cast<int>(sizeof(ProtocolMessage)))) {
            //TODO: HexDump(data)
            LOG_WARNING << "Truncated ProtocolMessage, len = " << len;
            return -1;
        }

        alpha::Slice received(data, len);

        //TODO: Check MagicNum
        auto * req = received.as<ProtocolMessage>()->as<const HandShakeRequest*>();
        //TODO: Check buffer_size
        ProtocolMessage m;
        auto * reply = m.as<HandShakeResponse*>();
        //TODO: check req->name
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
            ::strncpy(reply->output_tunnel_path, input_bus_path.data(), 
                    input_bus_path.size());
            ::strncpy(reply->input_tunnel_path, output_bus_path.data(), 
                    output_bus_path.size());
        }
        *out = alpha::Slice(&m).ToString();
        return 0;
    }

    int Server::HandleBusMessage(int64_t iteration) {
        bool busy = false;
        for (auto& p : clients_) {
            Message * m = nullptr;
            while (p.second->ReadMessage(&m)) {
                busy = true;
                auto source_address = Address::CreateDirectly(self_address_, 
                        p.second->addr().ClientAddress());
                m->SetSourceAddress(source_address);
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
        auto node_addr = m->RemoteAddress().NodeAddress();
        auto iter = out_links_.find(node_addr);
        auto data = alpha::Slice(reinterpret_cast<const char*>(m), m->Size());
        if (iter != out_links_.end()) {
            //已经建立过连接了
            iter->second->Write(data);
        } else {
            auto it = cached_node_addresses_.find(node_addr);
            if (it != cached_node_addresses_.end()) {
                //没有建立连接，但是有对方的网络位置
                client_->ConnectTo(it->second);
            } else {
                //也没有缓存对方的位置，请求一个
            }
        }
    }

    void Server::OnConnectedToResolveServer(alpha::TcpConnectionPtr& conn) {
        using namespace std::placeholders;
        conn->SetOnRead(std::bind(&ProtocolMessageCodec::OnRead,
                    protocol_message_codec_.get(), _1, _2));
        ProtocolMessage m;
        ResolveRequest::Builder builder(&m);
        builder.SetType(ResolveRequestType::kInitializeSelf);
        builder.SetPort(message_server_port_);
        conn->Write(alpha::Slice(reinterpret_cast<const char*>(&m), m.size()));
    }

    void Server::OnConnectedToResolveServerError(const alpha::NetAddress& addr) {
        LOG_WARNING << "Connect to resolve server(" << addr <<") failed";
        ReconnectToResolveServer(addr);
    }

    void Server::HandleResolveServerMessage(alpha::TcpConnectionPtr& conn, 
            const ProtocolMessage& m) {
        if (m.type != MessageType::kResolveResponse) {
            LOG_WARNING << "Invalid message type = " << (int)m.type;
            conn->Close();
            return;
        }

        auto * resp = m.as<const ResolveResponse*>();
        bool preserve_connection = false;
        switch (resp->type) {
            case ResolveResponseType::kUpdateSelf:
                preserve_connection = UpdateSelfAddress(resp);
                return;
            case ResolveResponseType::kUpdateCache:
                preserve_connection = UpdateNodeAddressCache(resp);
                return;
            default:
                LOG_INFO << "Unknown ResolveResponseType = " << (int)resp->type;
                return;
        }

        if (!preserve_connection) {
            conn->Close();
        }
    }

    void Server::OnResolveServerDisconnected(alpha::TcpConnectionPtr& conn) {
        ReconnectToResolveServer(conn->PeerAddr());
    }

    void Server::ReconnectToResolveServer(const alpha::NetAddress& addr) {
        static const int kRetryInterval = 2000; //2s
        loop_->RunAfter(kRetryInterval, std::bind(&alpha::TcpClient::ConnectTo,
                    client_.get(), addr));
    }

    bool Server::UpdateSelfAddress(const ResolveResponse* resp) {
        //FIXME: 检查node_path
        auto addr = Address::Create(resp->node_path);
        if (addr.NodeAddress() != resp->node_addr) {
            LOG_ERROR << "Mismatch address, resp->node_addr = " << resp->node_addr
                << ", node_addr = " << addr.NodeAddress() 
                << ", node_path = " << resp->node_path;
            return false;
        }
        self_address_ = resp->node_addr;
        LOG_INFO << "self_address_ = " << self_address_
            << ", self_path = " << resp->node_path;
        return true;
    }

    bool Server::UpdateNodeAddressCache(const ResolveResponse* resp) {
        auto node_addr = resp->node_addr;
        alpha::NetAddress net_addr(resp->node_ip, resp->node_port);

        auto it = cached_node_addresses_.find(node_addr);
        if (it == cached_node_addresses_.end()) {
            LOG_INFO << "Add new cache address, node_addr = " << node_addr
                << ", path = " << resp->node_path
                << ", net_addr = " << net_addr;
        } else {
            LOG_INFO << "update cache address, node_addr = " << node_addr
                << ", path = " << resp->node_path
                << ", old net_addr = " << it->second
                << ", new net_addr = " << net_addr;
        }
        return true;
    }

    std::string Server::GetInputBusPath(alpha::Slice name) const {
        return GetBusPath("%s/%s_input.bus", name);
    }

    std::string Server::GetOutputBusPath(alpha::Slice name) const {
        return GetBusPath("%s/%s_output.bus", name);
    }

    std::string Server::GetBusPath(alpha::Slice fmt, alpha::Slice name) const {
        char buf[kMaxPathSize];
        LOG_WARNING_IF(bus_location_.size() + name.size() 
                >= static_cast<size_t>(kMaxPathSize)) <<
            "length of path exceed kMaxPathSize, bus_location_ = " << bus_location_
            << ", name = " << name.data();
        snprintf(buf, sizeof(buf), fmt.data(), bus_location_.data(), name.data());
        DLOG_INFO << "name = " << name.data()
            << ", path = " << buf;
        return buf;
    }
}
