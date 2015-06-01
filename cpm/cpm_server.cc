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

#include <gflags/gflags.h>
#include <alpha/compiler.h>
#include <alpha/logger.h>
#include <alpha/process_bus.h>
#include <alpha/net_address.h>
#include <alpha/event_loop.h>
#include <alpha/tcp_client.h>
#include <alpha/tcp_server.h>
#include <alpha/udp_server.h>
#include "cpm_node.h"
#include "cpm_message.h"
#include "cpm_protocol.h"
#include "cpm_client_info.h"
#include "cpm_message_codec.h"
#include "cpm_protocol_message_codec.h"

DEFINE_int32(register_server_port, 8123, "Register server port(udp)");
DEFINE_string(message_server_ip, "0.0.0.0", "Message server ip");
DEFINE_int32(message_server_port, 8123, "Message server port(tcp)");
DEFINE_string(resolve_server_ip, "127.0.0.1", "Resolve server ip");
DEFINE_int32(resolve_server_port, 9999, "Resolve server port");

namespace cpm {
    Server::Server(alpha::EventLoop* loop, alpha::Slice bus_location)
        :register_server_port_(FLAGS_register_server_port)
         ,message_server_port_(FLAGS_message_server_port)
         ,self_address_(Address::kLocalNodeAddress) 
         ,message_server_ip_(FLAGS_message_server_ip)
         ,loop_(loop), bus_location_(bus_location.ToString()) {
    }

    Server::~Server() = default;

    bool Server::Run() {
        using namespace std::placeholders;
        loop_->set_cron_functor(std::bind(&Server::HandleBusMessage, this, _1));
        message_codec_.reset (new MessageCodec());
        message_codec_->SetOnMessage(std::bind(
            &Server::HandleRemoteNodeMessage, this, _1, _2));
        protocol_message_codec_.reset (new ProtocolMessageCodec());
        protocol_message_codec_->SetOnMessage(std::bind(
                    &Server::HandleResolveServerMessage, this, _1, _2));

        register_server_.reset (new alpha::UdpServer(loop_));
        bool ok = register_server_->Start(
                alpha::NetAddress("127.0.0.1", register_server_port_),
                std::bind(&Server::HandleInitCommand, this, _1, _2));
        if (!ok) {
            return false;
        }

        message_server_.reset (new alpha::TcpServer(loop_, 
                    alpha::NetAddress(message_server_ip_, message_server_port_)));
        message_server_->SetOnRead(std::bind(
                    &MessageCodec::OnRead, message_codec_.get(), _1, _2));
        if (!message_server_->Run()) {
            return false;
        }

        resolve_server_address_.reset (new alpha::NetAddress(
                    FLAGS_resolve_server_ip, FLAGS_resolve_server_port));
        client_.reset (new alpha::TcpClient(loop_));
        client_->SetOnConnected(std::bind(
                    &Server::OnConnectedToRemote, this, _1));
        client_->SetOnConnectError(std::bind(
                    &Server::OnConnectToRemoteError, this, _1));
        client_->SetOnClose(std::bind(
                    &Server::OnConnectToRemoteClose, this, _1));
        client_->ConnectTo(*resolve_server_address_);

        loop_->RunEvery(1000, [this]{
            LOG_INFO << "Connected nodes: " << connected_nodes_;
        });
        LOG_INFO << "Register server at [UDP]127.0.0.1:" << register_server_port_;
        LOG_INFO << "Message server at [TCP]" << message_server_ip_ 
            << ":" << message_server_port_;
        LOG_INFO << "Resolve server is [TCP]" << *resolve_server_address_;
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
        auto node = FindNodeByNetAddress(conn->PeerAddr());
        assert (node);
        node->SetConnection(conn);
    }

    void Server::OnConnectToRemoteNodeError(const alpha::NetAddress& addr) {
        LOG_WARNING << "Connect to remote node " << addr << " failed";
        // TODO: 几次之后清空缓存
        Node* node = FindNodeByNetAddress(addr);
        assert (node);
        node->SetConnecting(false);
        UnRegisterNode(node);
        //TODO: 重试几次？
    }

    void Server::OnRemoteNodeConnected(alpha::TcpConnectionPtr conn) {
        ++connected_nodes_;
    }

    void Server::OnRemoteNodeDisconnected(alpha::TcpConnectionPtr& conn) {
        --connected_nodes_;
        LOG_WARNING << "Remote node disconnected, net_address = " << conn->PeerAddr();
        auto node = FindNodeByNetAddress(conn->PeerAddr());
        assert (node);
        node->ClearConnection();
        UnRegisterNode(node);
    }

    bool Server::IsResolveServerAddress(const alpha::NetAddress& addr) {
        return addr == *resolve_server_address_;
    }

    ssize_t Server::HandleInitCommand(alpha::Slice data, char* out) {
        if (unlikely(data.size() != static_cast<int>(sizeof(ProtocolMessage)))) {
            //TODO: HexDump(data)
            LOG_WARNING << "Invalid ProtocolMessage, len = " << data.size();
            return -1;
        }

        //TODO: Check MagicNum
        auto * req = data.as<ProtocolMessage>()->as<const HandShakeRequest*>();
        //TODO: Check buffer_size
        ProtocolMessage* m = reinterpret_cast<ProtocolMessage*>(out);
        auto * reply = m->as<HandShakeResponse*>();
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
                    sizeof(reply->output_tunnel_path));
            ::strncpy(reply->input_tunnel_path, output_bus_path.data(), 
                    sizeof(reply->input_tunnel_path));
        }
        return sizeof(ProtocolMessage);
    }

    bool Server::HandleRemoteMessage(const Message* m) {
        auto remote_address = m->RemoteAddress();
        if (remote_address.NodeAddress() != self_address_) {
            LOG_WARNING << "Receive unexpected message"
                << ", m->RemoteAddress().NodeAddress() = " << remote_address.NodeAddress()
                << ", self_address_ = " << self_address_
                << ", m->Size() = " << m->Size();
            return false;
        }

        auto client_address = m->RemoteAddress().ClientAddress();
        auto it = clients_.find(client_address);
        if (it == clients_.end()) {
            LOG_INFO << "Drop message from " << m->SourceAddress().ToString()
                << " to " << m->RemoteAddress().ToString()
                << ", loacl client is offline";
        } else {
            bool ok = it->second->WriteMessage(m);
            LOG_INFO_IF(!ok) << "Drop message from " << m->SourceAddress().ToString()
                << " to " << it->second->name()
                << ", loacl client is full";
            DLOG_INFO_IF(ok) << "Forward 1 message from " << m->SourceAddress().ToString()
                << ", to " << it->second->name();
        }
        return true;
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
                if (remote_addr.NodeAddress() == self_address_ 
                        || remote_addr.NodeAddress() == 0) {
                    HandleToLocalMessage(m);
                } else {
                    HandleToRemoteMessage(m);
                }
            }
        }
        return busy ? alpha::EventLoop::kBusy : alpha::EventLoop::kIdle;
    }

    void Server::HandleToLocalMessage(Message* m) {
        assert (m->RemoteAddress().NodeAddress() == self_address_
                || m->RemoteAddress().NodeAddress() == 0);
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
            DLOG_INFO_IF(ok) << "Forward 1 message from " << source_name
                << ", to " << it->second->name();
        }
    }

    void Server::HandleToRemoteMessage(Message* m) {
        LOG_INFO << "HandleToRemoteMessage, m->RemoteAddress() = "
            << m->RemoteAddress().ToString();
        auto node_address = m->RemoteAddress().NodeAddress();
        auto node = FindNode(node_address);
        auto droped = Message::Default();
        if (node && !node->NotResolved()) {
            if (node->Connected()) {
                node->SendMessage(*m, &droped);
            } else if (node->Connecting()) {
                node->CacheMessage(*m, &droped);
            } else if (node->Resolving()) {
                node->CacheMessage(*m, &droped);
            } else if (node->Resolved()) {
                LOG_INFO << "Try connecto node, " << node->NetAddress();
                RegisterNode(node);
                client_->ConnectTo(node->NetAddress());
                node->SetConnecting(true);
            } else {
            }
        } else {
            if (!node) {
                node = AddNewNode(node_address);
            }
            assert (node->NotResolved());
            if (resolve_server_conn_) {
                //请求对应节点网络地址
                ReqeustNodeNetAddress(node_address);
                node->SetResolveState(Node::ResolveState::kResolving);
            } else {
                LOG_WARNING << "No connection to resolve server found";
            }

            node->CacheMessage(*m, &droped);
        }
        LOG_WARNING_IF(!droped.Empty()) << "Drop message from local client(" 
            << m->SourceAddress().ClientAddress()
            << ") to remote " << m->RemoteAddress().ToString()
            << ", size = " << droped.Size();
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
        resolve_server_conn_ = conn;
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
        DLOG_INFO << "resp->type = " << static_cast<int>(resp->type);
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

    void Server::HandleRemoteNodeMessage(alpha::TcpConnectionPtr& conn, const Message& m) {
        bool preserve_connection = HandleRemoteMessage(&m);
        if (!preserve_connection) {
            conn->Close();
        }
    }

    void Server::OnResolveServerDisconnected(alpha::TcpConnectionPtr& conn) {
        LOG_WARNING << "Resolve server disconnected, addr = " << conn->PeerAddr();
        ReconnectToResolveServer(conn->PeerAddr());
        resolve_server_conn_.reset();
    }

    void Server::ReconnectToResolveServer(const alpha::NetAddress& addr) {
        static const int kRetryInterval = 2000; //2s
        loop_->RunAfter(kRetryInterval, std::bind(&alpha::TcpClient::ConnectTo,
                    client_.get(), addr, false));
    }

    void Server::ReqeustNodeNetAddress(Address::NodeAddressType node_address) {
        DLOG_INFO << "node_address = " << node_address;
        if (resolve_server_conn_ && !resolve_server_conn_->closed()) {
            ProtocolMessage m;
            auto builder = ResolveRequest::Builder(&m);
            builder.SetSelfAddress(self_address_);
            builder.SetType(ResolveRequestType::kLookupNode);
            builder.SetPeerAddress(node_address);
            resolve_server_conn_->Write(m.Serialize());
            DLOG_INFO << "Send ResolveRequest, type = ResolveRequestType::kLookupNode";
        }
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
        auto node_address = resp->node_addr;
        auto node = FindNode(node_address);
        alpha::NetAddress net_address(resp->node_ip, resp->node_port);
        if (node && node->Resolved()) {
            if (resp->code == ResolveServerError::kOk) {
                LOG_INFO << "update cache address, node_address = " << node_address
                    << ", path = " << resp->node_path
                    << ", old net_addr = " << node->NetAddress()
                    << ", new net_addr = " << net_address;
                node->SetNetAddress(net_address);
            } else {
                switch (resp->code) {
                    case ResolveServerError::kInvalidNode:
                        LOG_INFO << "Invaid node, node_address = " << node_address;
                        break;
                    case ResolveServerError::kNodeNotFound:
                        LOG_INFO << "Node not found, node_address = " << node_address;
                        break;
                    default:
                        LOG_WARNING << "Unknown resp->code = "
                            << static_cast<int>(resp->code);
                        break;
                }
                node->SetResolveState(Node::ResolveState::kNotResolved);
            }
        } else {
            if (!node) {
                //Resolve Server刷新cpmd地址缓存
                assert (resp->code == ResolveServerError::kOk);
                node = AddNewNode(node_address);
                node->SetNetAddress(net_address);
                LOG_INFO << "Add new cache address, node_address = " << node_address
                    << ", path = " << resp->node_path
                    << ", net_addr = " << net_address;
            } else {
                assert (!node->Resolved());
                if (resp->code == ResolveServerError::kOk) {
                    node->SetNetAddress(net_address);
                    LOG_INFO << "Resolved node_address = " << node_address 
                        << ", net_address = " << net_address
                        << ", readable_address = " << resp->node_path;
                } else {
                    switch (resp->code) {
                        case ResolveServerError::kInvalidNode:
                            LOG_INFO << "Invaid node, node_address = " << node_address;
                            break;
                        case ResolveServerError::kNodeNotFound:
                            LOG_INFO << "Node not found, node_address = " << node_address;
                            break;
                        default:
                            LOG_WARNING << "Unknown resp->code = "
                                << static_cast<int>(resp->code);
                            break;
                    }
                }
            }
        }

        assert (node);
        if (node->HasCachedMessage() && !node->Connected() && node->Resolved()) {
            client_->ConnectTo(node->NetAddress());
            RegisterNode(node);
        }
        return true;
    }

    void Server::RegisterNode(Node* node) {
        assert (node);
        auto res = nodes_index_.emplace(node->NetAddress(), node);
        assert (res.second);
        (void)res;
    }

    void Server::UnRegisterNode(Node* node) {
        assert (node);
        auto it = nodes_index_.find(node->NetAddress());
        assert (it != nodes_index_.end());
        nodes_index_.erase(it);
    }

    Node* Server::AddNewNode(Address::NodeAddressType node_address) {
        auto res = nodes_.emplace(node_address, NodePtr(new Node(node_address)));
        assert (res.second);
        return res.first->second.get();
    }

    Node* Server::FindNode(Address::NodeAddressType node_address) {
        auto it = nodes_.find(node_address);
        return it == nodes_.end() ? nullptr : it->second.get();
    }

    Node* Server::FindNodeByNetAddress(const alpha::NetAddress& net_address) {
        auto it = nodes_index_.find(net_address);
        return it == nodes_index_.end() ? nullptr : it->second;
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
