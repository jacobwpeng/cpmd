/*
 * ==============================================================================
 *
 *       Filename:  cpm_udp_socket.cc
 *        Created:  04/11/15 20:16:26
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "cpm_udp_socket.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cassert>

namespace cpm {
    UdpSocket::UdpSocket()
        :sock_(-1) {
    }

    UdpSocket::~UdpSocket() {
        if (sock_ != -1) {
            ::close(sock_);
        }
    }

    Status UdpSocket::ConnectTo(const alpha::NetAddress& addr) {
        assert (addr.ip() == "127.0.0.1");
        sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_ == -1) {
            ::perror("socket");
            return Status::kSocketError;
        }
        auto sock_addr = addr.ToSockAddr();
        int err = ::connect(sock_, reinterpret_cast<sockaddr*>(&sock_addr), 
                sizeof(sock_addr));
        if (err) {
            ::perror("connect");
            return Status::kSocketError;
        }
        return Status::kOk;
    }

    Status UdpSocket::Send(const alpha::Slice& data) {
        ssize_t nbytes = ::send(sock_, data.data(), data.size(), 0);
        if (nbytes == -1 || static_cast<size_t>(nbytes) != data.size()) {
            return Status::kSocketError;
        }
        return Status::kOk;
    }

    Status UdpSocket::Recv(std::string* out, int timeout_ms) {
        assert (out);
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock_, &fds);

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = timeout_ms % 1000 * 1000;

        int ret = ::select(sock_ + 1, &fds, NULL, NULL, &tv);
        if (ret == -1) {
            perror("select");
            return Status::kSocketError;
        } else if (ret == 0) {
            return Status::kSocketTimeout;
        } else {
            assert (FD_ISSET(sock_, &fds));
            char buf[1<<16];
            ssize_t nbytes = ::recv(sock_, buf, sizeof(buf), 0);
            if (nbytes == -1) {
                ::perror("recv");
                return Status::kSocketError;
            }
            out->assign(buf, nbytes);
            return Status::kOk;;
        }
    }

    Status UdpSocket::SendAndRecv(const alpha::Slice& in, std::string* out, int timeout_ms) {
        auto status = Send(in);
        if (status != Status::kOk) {
            return status;
        }
        return Recv(out, timeout_ms);
    }
}
