/*
 * ==============================================================================
 *
 *       Filename:  cpm_impl.cc
 *        Created:  04/11/15 20:00:44
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "cpm_impl.h"
#include "cpm_protocol.h"
#include "cpm_udp_socket.h"
#include <alpha/compiler.h>
#include <cassert>
#include <cstring>

namespace cpm {
    ClientImpl::ClientImpl(alpha::Slice name, Options options)
        :name_(name.ToString()), options_(options) {
    }

    ClientImpl::~ClientImpl() = default;

    Status ClientImpl::Init() {
        UdpSocket sock;
        auto status = sock.ConnectTo(alpha::NetAddress("127.0.0.1", options_.port()));
        if (status != Status::kOk) {
            return status;
        }
        ProtocolMessage m;
        auto * req = m.as<HandShakeRequest*>();
        assert (name_.size() <= kMaxNameSize - 1);
        ::strncpy(req->name, name_.data(), kMaxNameSize);
        req->buffer_size = options_.buffer_size();
        ProtocolMessage reply;
        status = sock.SendAndRecv(m, &reply, kDefaultTimeout);
        if (status != Status::kOk) {
            return status;
        }

        auto * resp = reply.as<HandShakeResponse*>();

        input_ = std::move(alpha::ProcessBus::RestoreFrom(resp->input_tunnel_path,
                    options_.buffer_size()));
        output_ = std::move(alpha::ProcessBus::RestoreFrom(resp->output_tunnel_path,
                    options_.buffer_size()));
        if (input_ == nullptr || output_ == nullptr) {
            return Status::kCreateBusFailed;
        }
        return Status::kOk;
    }

    Status ClientImpl::SendMessage(const Message* m) {
        bool ok = output_->Write(reinterpret_cast<const char*>(m), m->Size());
        return ok ? Status::kOk : Status::kBusFull;
    }

    Status ClientImpl::ReceiveMessage(Message** m) {
        assert (m);
        int len;
        char* data = input_->Read(&len);
        if (data) {
            if (unlikely(len < Message::kMinSize)) {
                return Status::kTruncatedMessage;
            } else if (unlikely(len >= Message::kMaxSize)) {
                return Status::kInvalidMessage;
            }
            *m = reinterpret_cast<Message*>(data);
            return Status::kOk;
        }
        return Status::kBusEmpty;
    }

    Status Client::Create(Client** client, alpha::Slice name, Options options) {
        std::unique_ptr<ClientImpl> impl(new ClientImpl(name, options));
        auto status = impl->Init();
        if (status == Status::kOk) {
            assert (client);
            *client = impl.release();
            return Status::kOk;
        } else {
            return status;
        }
    }

    void Client::Destroy(Client* client) {
        delete client;
    }
}
