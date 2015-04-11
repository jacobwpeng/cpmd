/*
 * ==============================================================================
 *
 *       Filename:  cpm_udp_socket.h
 *        Created:  04/11/15 20:13:22
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __CPM_UDP_SOCKET_H__
#define  __CPM_UDP_SOCKET_H__

#include "cpm.h"
#include <cstring>
#include <string>
#include <type_traits>
#include <alpha/net_address.h>

namespace cpm {
    class UdpSocket {
        public:
            UdpSocket();
            ~UdpSocket();

            Status ConnectTo(const alpha::NetAddress& addr);
            Status Send(const alpha::Slice& in);
            Status Recv(std::string* out, int timeout_ms = -1);
            Status SendAndRecv(const alpha::Slice& in, std::string* out, int timeout_ms = -1);

            template<typename SendType, typename ReceiveType>
            typename std::enable_if<std::is_pod<SendType>::value 
                                && std::is_pod<ReceiveType>::value, Status>::type 
                    SendAndRecv(const SendType& in, ReceiveType* out, int timeout_ms = -1) {
                std::string buf;
                auto status = SendAndRecv(alpha::Slice(reinterpret_cast<const char*>(&in), sizeof(in)), 
                        &buf, timeout_ms);
                if (status != Status::kOk) {
                    return status;
                }
                if (buf.size() != sizeof(ReceiveType)) {
                    return Status::kInvalidReply;
                } else {
                    ::memcpy(out, buf.data(), buf.size());
                    return Status::kOk;
                }
            }

        private:
            int sock_;
    };
}

#endif   /* ----- #ifndef __CPM_UDP_SOCKET_H__  ----- */
