/*
 * =============================================================================
 *
 *       Filename:  port_mapper_command_server.cc
 *        Created:  03/30/15 14:18:23
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "port_mapper_command_server.h"
#include <functional>
#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include <alpha/udp_server.h>
#include <alpha/process_bus.h>
#include "cpmd.h"
#include "port_mapper_message_dispatcher.h"

namespace cpmd {
    CommandServer::CommandServer(alpha::EventLoop* loop, MessageDispatcher* m)
        :port_(-1), loop_(loop), dispatcher_(m) {
        assert (loop);
        assert (m);
    }

    CommandServer::~CommandServer() = default;

    int CommandServer::Init(const alpha::Slice& file) {
        port_ = 8123;
        data_path_ = "/tmp/";
        char last_char = *data_path_.rbegin();
        if (last_char == '/') {
            data_path_ = data_path_.substr(0, data_path_.size() - 1);
        }
        return 0;
    }

    void CommandServer::Run() {
        using namespace std::placeholders;
        server_.reset (new alpha::UdpServer(loop_, "127.0.0.1", port_));
        server_->set_read_callback(std::bind(&CommandServer::OnMessage,
                    this, _1, _2, _3));
        server_->Start();
        assert (server_);
    }

    int CommandServer::OnMessage(const char* in, size_t len, std::string* out) {
        if (len != sizeof(ProtocolMessage)) {
            LOG_WARNING << "Invalid size of message, len = " << len;
            return -1;
        }

        const ProtocolMessage* req = reinterpret_cast<const ProtocolMessage*>(in);
        ProtocolMessage resp;

        LOG_INFO << "Message len = " << len << ", type = " <<
            static_cast<int>(req->type);
        if (req->type == MessageType::kHandShake) {
            const HandShakeRequest* handshake_req = 
                reinterpret_cast<const HandShakeRequest*>(req->data);
            HandShakeResponse* handshake_resp = reinterpret_cast<HandShakeResponse*>(
                    resp.data);

            OnHandShake(handshake_req, handshake_resp);
        } else if (req->type == MessageType::kNameLookup) {
            const NameLookupRequest* name_lookup_req = 
                reinterpret_cast<const NameLookupRequest*>(req->data);
            NameLookupResponse* name_lookup_resp = reinterpret_cast<NameLookupResponse*>(
                    resp.data);

            OnNameLookup(name_lookup_req, name_lookup_resp);
        } else {
            LOG_WARNING << "Unknown message type: " << static_cast<int>(req->type);
            return -2;
        }

        resp.type = req->type;
        out->assign(reinterpret_cast<const char*>(&resp), sizeof(resp));
        return 0;
    }

    void CommandServer::OnHandShake(const HandShakeRequest* req, 
            HandShakeResponse* resp) {
        ::snprintf(resp->output_tunnel_path, kMaxPathSize - 1, "%s/%s_out.bus", 
                data_path_.data(), req->name);
        auto client_output = std::move(alpha::ProcessBus::RestoreOrCreate(
                    resp->output_tunnel_path, req->buffer_size, true));
        if (client_output == nullptr) {
            resp->code = HandShakeError::kFailedToCreateBus;
            LOG_WARNING << "RestoreOrCreate client output bus failed, path = "
                        << resp->output_tunnel_path;
            return;
        }

        ::snprintf(resp->input_tunnel_path, kMaxPathSize - 1, "%s/%s_in.bus", 
                data_path_.data(), req->name);
        auto client_input = std::move(alpha::ProcessBus::RestoreOrCreate(
                    resp->input_tunnel_path, req->buffer_size, true));
        if (client_input == nullptr) {
            resp->code = HandShakeError::kFailedToCreateBus;
            LOG_WARNING << "RestoreOrCreate client input bus failed, path = "
                        << resp->input_tunnel_path;
            return;
        }
        resp->code = HandShakeError::kOk;
        Address addr = Address::Create(req->name);
        dispatcher_->AddClient(addr.HashCode(), std::move(client_input), 
                std::move(client_output));
    }

    void CommandServer::OnNameLookup(const NameLookupRequest* req, 
            NameLookupResponse* resp) {
        LOG_INFO << "req->name = " << req->name;
        Address addr = Address::Create(req->name);
        resp->addr = addr.HashCode();
    }
}
