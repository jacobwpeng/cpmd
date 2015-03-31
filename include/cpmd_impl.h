/*
 * ==============================================================================
 *
 *       Filename:  cpmd_impl.h
 *        Created:  03/22/15 16:06:23
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __CPMD_IMPL_H__
#define  __CPMD_IMPL_H__

#include "cpmd.h"
#include <memory>
#include <map>
#include <alpha/process_bus.h>

namespace cpmd {
    class PortMapperClientImpl : public PortMapperClient {
        public:
            PortMapperClientImpl();
            virtual ~PortMapperClientImpl();

            virtual Status Send(const Address& addr, const char* data, int len);
            virtual Status Receive(Address* addr, char** pdata, int* plen);

            virtual Status SendMessage(const Message& m);
            virtual Status ReceiveMessage(Message** m);

            virtual Status Whereis(const char* name, Address* addr);

            Status ShakeHand(const char* self_name, Options options);

        private:
            Status InitSock(int port, int timeout);
            Status Communicates(const ProtocolMessage& req, ProtocolMessage* res);
            Status SendAndRecv(const void* req, size_t req_size, 
                    void* resp, size_t resp_size);
            using ProcessBusPtr = std::unique_ptr<alpha::ProcessBus>;
            int sock_;
            ProcessBusPtr input_;
            ProcessBusPtr output_;
    };
}

#endif   /* ----- #ifndef __CPMD_IMPL_H__  ----- */
