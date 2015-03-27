/*
 * ==============================================================================
 *
 *       Filename:  cpmd_impl.cc
 *        Created:  03/22/15 15:52:15
 *         Author:  peng wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "cpmd.h"
#include "cpmd_impl.h"
#include "cpmd_protocol.h"

#include <cassert>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <memory>
#include <alpha/compiler.h>
#include <alpha/logger.h>

namespace cpmd {
    /* Status */
    Status::Status(Status::Code code)
        :code_(code), msg_("") {
    }

    Status Status::OK() {
        return Status();
    }

    Status Status::InvalidAddress(const alpha::Slice& msg) {
        return Error(Code::kInvalidAddress, msg);
    }

    Status Status::HandShakeError(const alpha::Slice& msg) {
        return Error(Code::kHandShakeError, msg);
    }

    Status Status::NoSpace(const alpha::Slice& msg) {
        return Error(Code::kNoSpaceLeft, msg);
    }

    Status Status::NoMessage() {
        return Error(Code::kNoMessage, "");
    }

    Status Status::Error(Status::Code code, const alpha::Slice& msg) {
        Status status(code);
        status.msg_ = msg.ToString();
        return status;
    }

    std::string Status::ToString() const {
        return msg_;
    }

    Options::Options()
        :port(8123), buffer_size(1 << 16) {
    }

    std::size_t Address::HashCode() const {
        return hash_code_;
    }

    /* PortMapperClient */
    PortMapperClient* PortMapperClient::Create(const char* name, Options options) {
        assert (name);
        PortMapperClientImpl * impl = new PortMapperClientImpl;
        Status status = impl->ShakeHand(name, options);
        if (!status.ok()) {
            LOG_ERROR << "ShakeHand failed, " << status.ToString();
            return NULL;
        }
        return impl;
    }

    void PortMapperClient::Destroy(PortMapperClient* client) {
        delete client;
    }

    PortMapperClient::~PortMapperClient() {
    }

    /* PortMapperClientImpl */
    PortMapperClientImpl::PortMapperClientImpl()
        :sock_(-1) {
    }
    PortMapperClientImpl::~PortMapperClientImpl() {
        if (sock_ != -1) {
            ::close(sock_);
        }
    }

    Status PortMapperClientImpl::SendMessage(const cpmd::Message& m) {
        assert(static_cast<std::size_t>(m.len) <= sizeof(m.data));
        bool ok = output_->Write(reinterpret_cast<const char*>(&m), 
                m.len + offsetof(Message, data));
        if (likely(ok)) {
            return Status::OK();
        } else {
            return Status::NoSpace("Write failed");
        }
    }

    Status PortMapperClientImpl::ReceiveMessage(cpmd::Message** m) {
        assert (m);
        int32_t len;
        char* buf = input_->Read(&len);
        if (buf) {
            assert (static_cast<std::size_t>(len) >= offsetof(Message, data));
            Message* message = reinterpret_cast<Message*>(buf);
            assert (static_cast<std::size_t>(message->len) <= sizeof(message->data));
            *m = message;
            return Status::OK();
        } else {
            return Status::NoMessage();
        }
    }

    Status PortMapperClientImpl::Send(const cpmd::Address& addr, 
            const char* data, int len) {
        assert (len >= 0);
        assert (static_cast<std::size_t>(len) <= kMaxMessageDataSize);
        Message m;
        m.addr = addr.HashCode();
        m.len = len;
        ::memcpy(m.data, data, len);
        return SendMessage(m);
    }

    Status PortMapperClientImpl::Receive(cpmd::Address* addr, 
            char** pdata, int* plen) {
        assert (pdata);
        assert (plen);

        Message* m = nullptr;
        Status status = ReceiveMessage(&m);
        if (status.ok()) {
            *pdata = m->data;
            *plen = m->len;
        }
        return status;
    }

    Status PortMapperClientImpl::Whereis(const char* name, Address* addr) {
        assert (sock_ != -1);
        NameLookupRequest req;
        ::strncpy(req.name, name, sizeof(req.name) - 1);
        req.name[sizeof(req.name) - 1] = '\0';
        NameLookupResponse resp;

        Status status = SendAndRecv(&req, sizeof(req), &resp, sizeof(resp));
        if (!status.ok()) {
            return status;
        }
        addr->hash_code_ = resp.addr;
        return Status::OK();
    }

    Status PortMapperClientImpl::InitSock(int port, int timeout) {
        assert (sock_ == -1);
        sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_ == -1) {
            return Status::HandShakeError("Create socket failed");
        }
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;

        int ret = ::setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        if (ret != 0) {
            return Status::HandShakeError("Set receive timeout error");
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = ::htons(port);
        server_addr.sin_addr.s_addr = ::inet_addr("127.0.0.1");

        ret = ::connect(sock_, reinterpret_cast<struct sockaddr*>(&server_addr),
                sizeof(server_addr));
        if (ret != 0) {
            return Status::HandShakeError("Connect error");
        }
        return Status::OK();
    }

    Status PortMapperClientImpl::ShakeHand(const char* self_name, Options options) {
        HandShakeRequest req;
        req.buffer_size = options.buffer_size;
        ::strncpy(req.name, self_name, sizeof(req.name) - 1);
        req.name[sizeof(req.name) - 1] = '\0';
        HandShakeResponse resp;

        Status status = InitSock(options.port, 200);
        if (!status.ok()) {
            return status;
        }

        status = SendAndRecv(&req, sizeof(req), &resp, sizeof(resp));
        if (!status.ok()) {
            return status;
        }

        if (resp.code == HandShakeError::kOk) {
            input_ = std::move(alpha::ProcessBus::RestoreFrom(resp.input_tunnel_path,
                        options.buffer_size));
            output_ = std::move(alpha::ProcessBus::RestoreFrom(resp.output_tunnel_path,
                        options.buffer_size));
            if (input_ == nullptr) {
                return Status::HandShakeError("Create input tunnel failed");
            }
            if (output_ == nullptr) {
                return Status::HandShakeError("Create input tunnel failed");
            }
            return Status::OK();
        } else if (resp.code == HandShakeError::kAlreadyRegistered) {
            return Status::HandShakeError("Name already registered");
        } else {
            return Status::HandShakeError("Unknown error");
        }
    }

    Status PortMapperClientImpl::SendAndRecv(const void* req, size_t req_size, 
            void* resp, size_t resp_size) {
        ssize_t n = ::send(sock_, req, req_size, 0);
        if (n == -1) {
            return Status::HandShakeError("Send error");
        }

        n = ::recv(sock_, resp, resp_size, 0);

        if (n == -1) {
            return Status::HandShakeError("Receive error");
        }

        if (n != static_cast<ssize_t>(resp_size)) {
            return Status::HandShakeError("truncated response");
        }
        return Status::OK();
    }
}
