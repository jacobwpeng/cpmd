/*
 * =============================================================================
 *
 *       Filename:  cpm_resolve_server.cc
 *        Created:  04/15/15 14:53:44
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "cpm_resolve_server.h"

#include <cassert>
#include <type_traits>
#include <alpha/compiler.h>
#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include <alpha/tcp_server.h>

#include "cpm_protocol.h"
#include "cpm_protocol_message_codec.h"
#include "cpm_resolve_server_conf.h"

namespace cpm {
    static_assert(!std::is_same<alpha::TimerManager::TimerId, 
            Address::NodeAddressType>::value, 
            "TimerId and NodeAddressType cannot be the same type");
    ResolveServer::ResolveServer(alpha::EventLoop* loop, const char* file)
        :loop_(loop) {
        conf_ = std::move(ResolveServerConf::CreateFromFile(file));
    }

    ResolveServer::~ResolveServer() = default;

    bool ResolveServer::Run() {
        if (!conf_) {
            LOG_WARNING << "Read ResolveServerConf failed";
            return false;
        }
        using namespace std::placeholders;
        protocol_message_codec_.reset (new ProtocolMessageCodec);
        auto service_address = conf_->service_address();
        assert (service_address);
        service_server_.reset (new alpha::TcpServer(loop_, *service_address));
        service_server_->SetOnRead(std::bind(&ProtocolMessageCodec::OnRead, 
                    protocol_message_codec_.get(), _1, _2));
        service_server_->SetOnNewConnection(std::bind(&ResolveServer::OnNewConnection, 
                    this, _1));
        service_server_->SetOnClose(std::bind(&ResolveServer::OnClose, this, _1));
        if (service_server_->Run() == false) {
            LOG_ERROR << "Run service server failed";
            return false;
        }
        return true;
    }

    void ResolveServer::OnNewConnection(alpha::TcpConnectionPtr conn) {
        //在连接成功后几秒之内必须请求自己的逻辑地址
        static const uint32_t kMaxWaitTime = 2000; // 2s
        auto timerid = loop_->RunAfter(kMaxWaitTime, std::bind(
                    &ResolveServer::KickOffNode, this, alpha::TcpConnectionWeakPtr(conn)));
        conn->SetContext(timerid);
    }

    void ResolveServer::OnClose(alpha::TcpConnectionPtr conn) {
        auto node_addr = conn->GetContext<Address::NodeAddressType>();
        if (node_addr) {
            assert (connections_.find(*node_addr) != connections_.end());
            connections_.erase(*node_addr);
        }
    }

    void ResolveServer::KickOffNode(alpha::TcpConnectionWeakPtr weak_conn) {
        if (auto conn = weak_conn.lock()) {
            conn->Close();
            LOG_INFO << "Kick off node, addr = " << conn->PeerAddr();
        }
    }

    void ResolveServer::OnMessage(alpha::TcpConnectionPtr& conn, const ProtocolMessage& m) {
        //第一个请求必须是请求初始化自己的请求
        auto req = m.as<const ResolveRequest*>();
        ProtocolMessage reply;
        auto builder = ResolveResponse::Builder(&reply);
        bool preserve_connection = true;
        if (req->type == ResolveRequestType::kInitializeSelf) {
            preserve_connection = HandleNodeInit(conn, req, &reply);
        } else if (req->type == ResolveRequestType::kLookupNode) {
            preserve_connection = HandleNodeLookup(conn, req, &reply);
        } else {
            LOG_WARNING << "Unknown req->type = " << static_cast<int>(req->type);
            preserve_connection = false;
        }
        conn->Write(alpha::Slice(reinterpret_cast<const char*>(&reply), reply.size()));
        if (!preserve_connection) {
            conn->Close();
        }
    }

    bool ResolveServer::HandleNodeInit(alpha::TcpConnectionPtr& conn, 
            const ResolveRequest* req, ProtocolMessage* reply) {
        auto builder = ResolveResponse::Builder(reply);
        alpha::NetAddress message_server_addr(conn->PeerAddr().ip(), req->cpmd_port);
        auto node = conf_->GetNodeInfoByNetAddress(message_server_addr);
        if (node == nullptr) {
            builder.SetCode(ResolveServerError::kNodeNotFound);
            LOG_INFO << "No node(" << message_server_addr << ") found in conf";
            return false;
        } else {
            builder.SetCode(ResolveServerError::kOk);
            builder.SetType(ResolveResponseType::kUpdateSelf);
            builder.SetNodeAddress(node->node_addr());
            builder.SetNodePort(req->cpmd_port);
            builder.SetNodeIp(conn->PeerAddr().ip());
            builder.SetNodePath(node->readable_addr());
            auto res = connections_.emplace(node->node_addr(), conn);
            assert (res.second);
            (void)res;
            auto timerid = conn->GetContext<alpha::TimerManager::TimerId>();
            assert (timerid);
            loop_->RemoveTimer(*timerid);
            conn->SetContext(node->node_addr());
            return true;
        }
    }

    bool ResolveServer::HandleNodeLookup(alpha::TcpConnectionPtr& conn, 
            const ResolveRequest* req, ProtocolMessage* reply) {
        //可能是timerid 或者是 node_addr
        assert (conn->HasContext());
        auto it = connections_.find(req->self_addr);
        if (it == connections_.end()) {
            return false;
        } else {
            auto node_addr = it->second->GetContext<Address::NodeAddressType>();
            //没有初始化的连接
            if (!node_addr) {
                return false;
            }
            //客户端发送过来的自己的地址错误
            if (it->first != *node_addr) {
                return false;
            }
        }
        auto builder = ResolveResponse::Builder(reply);
        //FIXME: check node_path is null-terminated
        //FIXME: check node_path is valid
        auto readable_addr = std::string(req->node_path);
        auto addr = cpm::Address::Create(readable_addr);
        auto node = conf_->GetNodeInfoByNodeAddress(addr.NodeAddress());
        if (node == nullptr) {
            builder.SetCode(ResolveServerError::kNodeNotFound);
        } else {
            assert (node->node_addr() == addr.NodeAddress());
            builder.SetCode(ResolveServerError::kOk);
            builder.SetType(ResolveResponseType::kUpdateCache);
            builder.SetNodeIp(node->net_addr().ip());
            builder.SetNodePort(node->net_addr().port());
            builder.SetNodeAddress(node->node_addr());
        }
        return true;
    }
}
