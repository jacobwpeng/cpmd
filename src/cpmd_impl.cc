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

    Status Status::InitError(const alpha::Slice& msg) {
        return Error(Code::kInitError, msg);
    }

    Status Status::InvalidAddress(const alpha::Slice& msg) {
        return Error(Code::kInvalidAddress, msg);
    }

    Status Status::NoSpace(const alpha::Slice& msg) {
        return Error(Code::kNoSpaceLeft, msg);
    }

    Status Status::NoMessage() {
        return Error(Code::kNoMessage, "");
    }
    
    Status Status::CommunicateError(const alpha::Slice& msg) {
        return Error(Code::kCommunicateError, msg);
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

    Address::Address()
        :hash_code_(0) {
    }

    uint32_t Address::HashCode() const {
        return hash_code_;
    }

    Address Address::Create(const alpha::Slice& slice) {
        // MurmurHash 32-bit
        static const uint32_t c1 = 0xcc9e2d51;
        static const uint32_t c2 = 0x1b873593;
        static const uint32_t r1 = 15;
        static const uint32_t r2 = 13;
        static const uint32_t m = 5;
        static const uint32_t n = 0xe6546b64;

        auto key = slice.data();
        uint32_t len = slice.size();
        static const uint32_t seed = 0xEE6B27EB;
     
        uint32_t hash = seed;
     
        const int nblocks = len / 4;
        const uint32_t *blocks = (const uint32_t *) key;
        int i;
        for (i = 0; i < nblocks; i++) {
            uint32_t k = blocks[i];
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;
     
            hash ^= k;
            hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
        }
     
        const uint8_t *tail = (const uint8_t *) (key + nblocks * 4);
        uint32_t k1 = 0;
     
        switch (len & 3) {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
     
            k1 *= c1;
            k1 = (k1 << r1) | (k1 >> (32 - r1));
            k1 *= c2;
            hash ^= k1;
        }
     
        hash ^= len;
        hash ^= (hash >> 16);
        hash *= 0x85ebca6b;
        hash ^= (hash >> 13);
        hash *= 0xc2b2ae35;
        hash ^= (hash >> 16);
     
        return Address(hash);
    }

    Address::Address(uint32_t hash_code)
        :hash_code_(hash_code) {
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
        ProtocolMessage req_wrapper, resp_wrapper;
        NameLookupRequest* req = reinterpret_cast<NameLookupRequest*>(req_wrapper.data);
        req_wrapper.type = MessageType::kNameLookup;
        ::strncpy(req->name, name, sizeof(req->name) - 1);
        req->name[sizeof(req->name) - 1] = '\0';

        Status status = Communicates(req_wrapper, &resp_wrapper);
        if (!status.ok()) {
            return status;
        }
        assert (resp_wrapper.type == MessageType::kNameLookup);
        NameLookupResponse* resp = reinterpret_cast<NameLookupResponse*>(resp_wrapper.data);
        addr->hash_code_ = resp->addr;
        return Status::OK();
    }

    Status PortMapperClientImpl::InitSock(int port, int timeout) {
        assert (sock_ == -1);
        sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_ == -1) {
            return Status::InitError("Create socket failed");
        }
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;

        int ret = ::setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        if (ret != 0) {
            return Status::InitError("Set receive timeout error");
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = ::htons(port);
        server_addr.sin_addr.s_addr = ::inet_addr("127.0.0.1");

        ret = ::connect(sock_, reinterpret_cast<struct sockaddr*>(&server_addr),
                sizeof(server_addr));
        if (ret != 0) {
            return Status::InitError("Connect error");
        }
        return Status::OK();
    }

    Status PortMapperClientImpl::ShakeHand(const char* self_name, Options options) {
        ProtocolMessage req;
        HandShakeRequest * p = reinterpret_cast<HandShakeRequest*>(req.data);
        req.type = MessageType::kHandShake;
        p->buffer_size = options.buffer_size;
        ::strncpy(p->name, self_name, sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = '\0';

        Status status = InitSock(options.port, 200);
        if (!status.ok()) {
            return status;
        }

        ProtocolMessage resp_wrapper;
        status = Communicates(req, &resp_wrapper);
        if (!status.ok()) {
            return status;
        }
        assert (resp_wrapper.type == MessageType::kHandShake);
        HandShakeResponse * resp = reinterpret_cast<HandShakeResponse*>(resp_wrapper.data);

        if (resp->code == HandShakeError::kOk) {
            input_ = std::move(alpha::ProcessBus::RestoreFrom(resp->input_tunnel_path,
                        options.buffer_size));
            output_ = std::move(alpha::ProcessBus::RestoreFrom(resp->output_tunnel_path,
                        options.buffer_size));
            if (input_ == nullptr) {
                return Status::InitError("Create input tunnel failed");
            }
            if (output_ == nullptr) {
                return Status::InitError("Create input tunnel failed");
            }
            return Status::OK();
        } else if (resp->code == HandShakeError::kAlreadyRegistered) {
            return Status::InitError("Name already registered");
        } else {
            return Status::InitError("Unknown error");
        }
    }

    Status PortMapperClientImpl::Communicates(const ProtocolMessage& req, 
            ProtocolMessage* resp) {
        assert (resp);
        ssize_t n = ::send(sock_, &req, sizeof(ProtocolMessage), 0);
        if (n == -1) {
            return Status::CommunicateError("send error");
        }

        n = ::recv(sock_, resp, sizeof(ProtocolMessage), 0);
        if (n == -1) {
            return Status::CommunicateError("recv error");
        }

        if (n != static_cast<ssize_t>(sizeof(ProtocolMessage))) {
            return Status::CommunicateError("truncated response");
        }

        return Status::OK();
    }
}
