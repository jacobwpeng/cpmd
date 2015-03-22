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

#include <cassert>

namespace cpmd {
    /* Status */
    Status::Status(Status::Code code)
        :code_(code), msg_(NULL) {
    }

    Status Status::OK() {
        return Status();
    }

    Status Status::Timeout(const char* msg) {
        Status status(Code::kTimeout);
        status.msg_ = msg;
        return status;
    }

    Status Status::InvalidAddress(const char* msg) {
        Status status(Code::kInvalidAddress);
        status.msg_ = msg;
        return status;
    }

    PortMapperClient* PortMapperClient::Create(const char* name, Options options) {
        assert (name);
        /* Send udp request */
        /* Receive udp response with timeout */
        /* Get bus path */
#if 0
        std::string out_tunnel_path;
        std::string in_tunnel_path;

        ProcessBus * out = CreateProcessBus(out_tunnel_path);
        ProcessBus * in = nullptr;
        if (!in_tunnel_path.empty()) {
            in = CreateProcessBus(in_tunnel_path);
        }
        assert (out);
        assert (in_tunnel_path.empty() || in);
        return new PortMapperClientImpl(out, in);
#endif
        return new PortMapperClientImpl();
    }

    /* PortMapperClient */
    void PortMapperClient::Destroy(PortMapperClient* client) {
        delete client;
    }

    /* PortMapperClientImpl */
    PortMapperClientImpl::~PortMapperClientImpl() {
    }

    Status PortMapperClientImpl::Send(const Address* addr, const char* data, std::size_t len) {
        assert (addr);
        assert (data);
        return Status::OK();
    }

    Status PortMapperClientImpl::Receive(Address** addr, const char** data, std::size_t* len) {
        assert (addr);
        assert (data);
        assert (len);
        return Status::OK();
    }
    Status PortMapperClientImpl::Whereis(const char* name, Address** addr) {
        assert (name);
        assert (addr);
        return Status::OK();
    }
}
